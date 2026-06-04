#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using Clock = std::chrono::high_resolution_clock;

static inline uint16_t be16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}

int main(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "data/sample.itch";

    int fd = open(path, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);
    const uint8_t* data = static_cast<const uint8_t*>(
        mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    close(fd);
    madvise(const_cast<uint8_t*>(data), sb.st_size, MADV_SEQUENTIAL);

    const uint8_t* p   = data;
    const uint8_t* end = data + sb.st_size;

    uint64_t adds = 0, deletes = 0, others = 0, messages = 0;

    auto t0 = Clock::now();

    while (p + 2 < end && messages < 500000) {
        uint16_t len  = be16(p);
        uint8_t  type = p[2];
        if (p + 2 + len > end) break;
        switch (type) {
            case 'A': adds++;    break;
            case 'D': deletes++; break;
            default:  others++;  break;
        }
        p += 2 + len;
        messages++;
    }

    auto t1       = Clock::now();
    auto ns       = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    munmap(const_cast<uint8_t*>(data), sb.st_size);

    std::cout << "--- Pure mmap Parse Speed ---\n";
    std::cout << "Messages scanned : " << messages      << "\n";
    std::cout << "Add orders       : " << adds          << "\n";
    std::cout << "Delete orders    : " << deletes        << "\n";
    std::cout << "Other types      : " << others        << "\n";
    std::cout << "Total time       : " << ns / 1000000  << " ms\n";
    std::cout << "Avg per message  : " << ns / messages << " ns\n";
    return 0;
}
