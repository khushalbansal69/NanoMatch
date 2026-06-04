#pragma once
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "matching_engine.h"
#include "memory_pool.h"

static constexpr uint8_t ITCH_ADD_ORDER    = 'A';
static constexpr uint8_t ITCH_DELETE_ORDER = 'D';
static constexpr uint8_t ITCH_ORDER_EXEC   = 'E';

static inline uint16_t be16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}
static inline uint32_t be32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]);
}
static inline uint64_t be64(const uint8_t* p) {
    return (static_cast<uint64_t>(be32(p)) << 32) | be32(p + 4);
}

class ITCHParser {
public:
    struct Stats {
        uint64_t add_orders     = 0;
        uint64_t delete_orders  = 0;
        uint64_t executions     = 0;
        uint64_t unknown        = 0;
        uint64_t bytes_parsed   = 0;
    };

    // Parse up to max_messages from an ITCH binary file via mmap.
    // Returns false if the file could not be opened.
    bool Parse(const char* path, MatchingEngine& engine,
               MemoryPool<Order, 500000>& pool,
               uint64_t max_messages = 500000) {

        int fd = open(path, O_RDONLY);
        if (fd < 0) return false;

        struct stat sb;
        if (fstat(fd, &sb) < 0) { close(fd); return false; }

        const uint8_t* data = static_cast<const uint8_t*>(
            mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        close(fd);
        if (data == MAP_FAILED) return false;

        madvise(const_cast<uint8_t*>(data), sb.st_size, MADV_SEQUENTIAL);

        const uint8_t* p   = data;
        const uint8_t* end = data + sb.st_size;
        uint64_t       msg_count = 0;

        while (p + 2 < end && msg_count < max_messages) {
            uint16_t      len  = be16(p);          // message length field
            const uint8_t type = p[2];             // message type byte
            const uint8_t* msg = p + 2;            // payload starts at type byte

            if (p + 2 + len > end) break;

            switch (type) {
                case ITCH_ADD_ORDER: {
                    // Add Order (A): 36 bytes
                    // [2] type [3-4] stock_locate [5-6] tracking
                    // [7-14] order_ref [15] buy_sell [16-19] shares
                    // [20-27] stock [28-31] price (1/10000 dollar)
                    if (len < 34) break;
                    uint64_t ref   = be64(msg + 5);
                    uint8_t  side  = msg[13];
                    uint32_t qty   = be32(msg + 14);
                    int64_t  price = static_cast<int64_t>(be32(msg + 26));

                    Order* o       = pool.Acquire();
                    o->order_id    = ref;
                    o->price       = price;
                    o->quantity    = qty;
                    o->remaining   = qty;
                    o->side        = (side == 'B') ? Side::BUY : Side::SELL;
                    o->type        = OrderType::LIMIT;
                    engine.ProcessOrder(o);
                    stats_.add_orders++;
                    break;
                }
                case ITCH_DELETE_ORDER: {
                    // Delete Order (D): 19 bytes
                    // [5-12] order_ref
                    if (len < 17) break;
                    uint64_t ref = be64(msg + 5);
                    engine.Book().Cancel(ref);
                    stats_.delete_orders++;
                    break;
                }
                case ITCH_ORDER_EXEC: {
                    // Order Executed (E): 31 bytes — count only
                    stats_.executions++;
                    break;
                }
                default:
                    stats_.unknown++;
                    break;
            }

            p += 2 + len;
            msg_count++;
        }

        stats_.bytes_parsed = p - data;
        munmap(const_cast<uint8_t*>(data), sb.st_size);
        return true;
    }

    const Stats& GetStats() const { return stats_; }

private:
    Stats stats_;
};
