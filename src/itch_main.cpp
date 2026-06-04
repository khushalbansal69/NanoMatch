#include <chrono>
#include <iostream>
#include <thread>
#include <atomic>
#include "itch_parser.h"
#include "matching_engine.h"
#include "memory_pool.h"

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "data/sample.itch";

    MemoryPool<Order, 500000> pool;
    MatchingEngine            engine;

    std::atomic<bool>     logger_running{true};
    std::atomic<uint64_t> logged_trades{0};

    std::thread logger([&]() {
        TradeEvent ev;
        while (logger_running.load(std::memory_order_relaxed) || !engine.Ring().Empty())
            if (engine.Ring().Pop(ev))
                logged_trades.fetch_add(1, std::memory_order_relaxed);
    });

    ITCHParser parser;
    auto t0 = Clock::now();
    bool ok  = parser.Parse(path, engine, pool, 500000);
    auto t1  = Clock::now();

    logger_running = false;
    logger.join();

    if (!ok) {
        std::cerr << "Failed to open: " << path << "\n";
        return 1;
    }

    auto& s      = parser.GetStats();
    auto  ms     = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto  ns_per = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
                   / std::max(s.add_orders, uint64_t(1));

    std::cout << "--- ITCH Parser Results ---\n";
    std::cout << "Add orders parsed  : " << s.add_orders       << "\n";
    std::cout << "Delete orders      : " << s.delete_orders     << "\n";
    std::cout << "Executions seen    : " << s.executions        << "\n";
    std::cout << "Bytes parsed       : " << s.bytes_parsed      << "\n";
    std::cout << "Trades generated   : " << engine.TradeCount() << "\n";
    std::cout << "Trades logged      : " << logged_trades.load()<< "\n";
    std::cout << "Total time         : " << ms                  << " ms\n";
    std::cout << "Avg per add order  : " << ns_per              << " ns\n";
    return 0;
}
