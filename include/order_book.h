#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "price_level.h"

class OrderBook {
public:
    void Add(Order* order) {
        auto& side = (order->side == Side::BUY) ? bids_ : asks_;

        for (auto& level : side) {
            if (level.price == order->price) {
                level.push(order);
                order_map_[order->order_id] = &level;
                return;
            }
        }

        side.emplace_back(order->price);
        side.back().push(order);
        order_map_[order->order_id] = &side.back();

        if (order->side == Side::BUY) {
            std::sort(bids_.begin(), bids_.end(),
                [](const PriceLevel& a, const PriceLevel& b) {
                    return a.price > b.price;
                });
        } else {
            std::sort(asks_.begin(), asks_.end(),
                [](const PriceLevel& a, const PriceLevel& b) {
                    return a.price < b.price;
                });
        }
    }

    bool Cancel(uint64_t order_id) {
        auto it = order_map_.find(order_id);
        if (it == order_map_.end()) return false;

        PriceLevel* level = it->second;
        auto& q = level->orders;
        for (auto qit = q.begin(); qit != q.end(); ++qit) {
            if ((*qit)->order_id == order_id) {
                q.erase(qit);
                order_map_.erase(it);
                CleanEmptyLevels();
                return true;
            }
        }
        return false;
    }

    PriceLevel* BestBid() { PruneFront(bids_); return bids_.empty() ? nullptr : &bids_.front(); }
    PriceLevel* BestAsk() { PruneFront(asks_); return asks_.empty() ? nullptr : &asks_.front(); }

    std::vector<PriceLevel>&       Bids()       { return bids_; }
    std::vector<PriceLevel>&       Asks()       { return asks_; }
    const std::vector<PriceLevel>& Bids() const { return bids_; }
    const std::vector<PriceLevel>& Asks() const { return asks_; }

    size_t BidLevels() const { return bids_.size(); }
    size_t AskLevels() const { return asks_.size(); }
    bool   Empty()     const { return bids_.empty() && asks_.empty(); }

private:
    void PruneFront(std::vector<PriceLevel>& side) {
        while (!side.empty() && side.front().empty())
            side.erase(side.begin());
    }

    void CleanEmptyLevels() {
        auto empty = [](const PriceLevel& l) { return l.empty(); };
        bids_.erase(std::remove_if(bids_.begin(), bids_.end(), empty), bids_.end());
        asks_.erase(std::remove_if(asks_.begin(), asks_.end(), empty), asks_.end());
    }

    std::vector<PriceLevel>                    bids_;
    std::vector<PriceLevel>                    asks_;
    std::unordered_map<uint64_t, PriceLevel*>  order_map_;
};
