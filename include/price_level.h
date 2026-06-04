#pragma once
#include <deque>
#include "order.h"

class PriceLevel {
public:
    int64_t            price;
    std::deque<Order*> orders;

    explicit PriceLevel(int64_t p) : price(p) {}

    void    push(Order* o) { orders.push_back(o); }
    Order*  front()        { return orders.empty() ? nullptr : orders.front(); }
    void    pop_front()    { if (!orders.empty()) orders.pop_front(); }
    bool    empty() const  { return orders.empty(); }

    uint32_t total_quantity() const {
        uint32_t total = 0;
        for (auto* o : orders) total += o->remaining;
        return total;
    }
};
