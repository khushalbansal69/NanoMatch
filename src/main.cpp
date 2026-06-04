#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "matching_engine.h"
#include "memory_pool.h"
#include "order.h"
#include "order_book.h"

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char* argv[]) {
    const std::string path = (argc > 1) ? argv[1] : "data/synthetic/orders.csv";

    MemoryPool<Order, 200000> pool;
    MatchingEngine            engine;

    std::atomic<bool>     logger_running{true};
    std::atomic<uint64_t> logged_trades{0};

    std::thread logger([&]() {
        TradeEvent ev;
        while (logger_running.load(std::memory_order_relaxed) || !engine.Ring().Empty())
            if (engine.Ring().Pop(ev))
                logged_trades.fetch_add(1, std::memory_order_relaxed);
    });

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open: " << path << "\n";
        logger_running = false;
        logger.join();
        return 1;
    }

    std::string line;
    std::getline(file, line);

    std::vector<long long> match_times;
    match_times.reserve(100000);
    long long parse_ns = 0;
    uint64_t  count    = 0;

    while (std::getline(file, line)) {
        auto t0 = Clock::now();

        std::stringstream ss(line);
        std::string tok;
        Order* o = pool.Acquire();

        std::getline(ss, tok, ','); o->order_id = std::stoull(tok);
        std::getline(ss, tok, ','); o->side     = (tok == "BUY") ? Side::BUY : Side::SELL;
        std::getline(ss, tok, ','); o->price    = std::stoll(tok);
        std::getline(ss, tok, ','); o->quantity = std::stoul(tok);
        std::getline(ss, tok, ',');
        o->type      = (tok.find("MARKET") != std::string::npos) ? OrderType::MARKET : OrderType::LIMIT;
        o->remaining = o->quantity;

        auto t1 = Clock::now();
        engine.ProcessOrder(o);
        auto t2 = Clock::now();

        parse_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        match_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
        ++count;
    }

    logger_running = false;
    logger.join();

    std::sort(match_times.begin(), match_times.end());
    auto pct = [&](double p) {
        return match_times[static_cast<size_t>(p / 100.0 * match_times.size())];
    };

    std::cout << "Orders processed : " << count                << "\n";
    std::cout << "Trades generated : " << engine.TradeCount()  << "\n";
    std::cout << "Trades logged    : " << logged_trades.load() << "\n";
    std::cout << "\n--- Match Latency ---\n";
    std::cout << "p50  : " << pct(50)    << " ns\n";
    std::cout << "p90  : " << pct(90)    << " ns\n";
    std::cout << "p99  : " << pct(99)    << " ns\n";
    std::cout << "p99.9: " << pct(99.9)  << " ns\n";
    std::cout << "parse: " << parse_ns / count << " ns avg\n";
    return 0;
}
