#pragma once
#include "order_book.h"
#include "ring_buffer.h"

struct TradeEvent {
    uint64_t aggressor_id;
    uint64_t resting_id;
    int64_t  price;
    uint32_t quantity;
};

class MatchingEngine {
public:
    static constexpr size_t RING_SIZE = 65536;

    void ProcessOrder(Order* order) {
        if (order->type == OrderType::LIMIT)
            ProcessLimit(order);
        else
            ProcessMarket(order);
    }

    SPSCRingBuffer<TradeEvent, RING_SIZE>& Ring()        { return ring_; }
    const OrderBook&                       Book()  const { return book_; }
    uint64_t                               TradeCount() const { return trade_count_; }

private:
    void Emit(uint64_t agg, uint64_t rest, int64_t price, uint32_t qty) {
        ring_.Push(TradeEvent{agg, rest, price, qty});
        ++trade_count_;
    }

    void ProcessLimit(Order* incoming) {
        while (incoming->remaining > 0) {
            PriceLevel* best = (incoming->side == Side::BUY)
                ? book_.BestAsk() : book_.BestBid();
            if (!best) break;

            bool crosses = (incoming->side == Side::BUY)
                ? incoming->price >= best->price
                : incoming->price <= best->price;
            if (!crosses) break;

            while (incoming->remaining > 0 && !best->orders.empty()) {
                Order*   resting = best->orders.front();
                uint32_t fill    = std::min(incoming->remaining, resting->remaining);
                Emit(incoming->order_id, resting->order_id, best->price, fill);
                incoming->remaining -= fill;
                resting->remaining  -= fill;
                if (resting->remaining == 0) best->orders.pop_front();
            }
        }
        if (incoming->remaining > 0) book_.Add(incoming);
    }

    void ProcessMarket(Order* incoming) {
        while (incoming->remaining > 0) {
            PriceLevel* best = (incoming->side == Side::BUY)
                ? book_.BestAsk() : book_.BestBid();
            if (!best) break;

            while (incoming->remaining > 0 && !best->orders.empty()) {
                Order*   resting = best->orders.front();
                uint32_t fill    = std::min(incoming->remaining, resting->remaining);
                Emit(incoming->order_id, resting->order_id, best->price, fill);
                incoming->remaining -= fill;
                resting->remaining  -= fill;
                if (resting->remaining == 0) best->orders.pop_front();
            }
        }
    }

    OrderBook                          book_;
    SPSCRingBuffer<TradeEvent, RING_SIZE> ring_;
    uint64_t                           trade_count_ = 0;
};
