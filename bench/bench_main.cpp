#include <benchmark/benchmark.h>
#include "matching_engine.h"
#include "memory_pool.h"
#include "order.h"
#include "order_book.h"

static void BM_OrderRest(benchmark::State& state) {
    MatchingEngine engine;
    uint64_t id = 1;
    for (auto _ : state) {
        Order o{id++, static_cast<int64_t>(10000 + id % 100), 100, 100,
                Side::BUY, OrderType::LIMIT, {}};
        engine.ProcessOrder(&o);
        benchmark::DoNotOptimize(o);
    }
}
BENCHMARK(BM_OrderRest)->Iterations(100000);

static void BM_OrderMatch(benchmark::State& state) {
    uint64_t id = 1;
    for (auto _ : state) {
        MatchingEngine engine;
        Order sell{id++, 10000, 100, 100, Side::SELL, OrderType::LIMIT, {}};
        Order buy {id++, 10000, 100, 100, Side::BUY,  OrderType::LIMIT, {}};
        engine.ProcessOrder(&sell);
        engine.ProcessOrder(&buy);
        benchmark::DoNotOptimize(buy);
    }
}
BENCHMARK(BM_OrderMatch)->Iterations(50000);

static void BM_RingBuffer(benchmark::State& state) {
    SPSCRingBuffer<TradeEvent, 65536> ring;
    TradeEvent ev{1, 2, 10000, 100};
    TradeEvent out;
    for (auto _ : state) {
        ring.Push(ev);
        ring.Pop(out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_RingBuffer)->Iterations(500000);

BENCHMARK_MAIN();
