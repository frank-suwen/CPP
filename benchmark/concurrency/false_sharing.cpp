#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr std::int64_t ITERATIONS = 100'000'000;
constexpr int NUM_THREADS = 4;

struct CompactCounter {
    std::atomic<std::int64_t> value{0};
};

struct alignas(64) PaddedCounter {
    std::atomic<std::int64_t> value{0};
};

template <typename Counter>
void run_benchmark(const char* name) {
    std::vector<Counter> counters(NUM_THREADS);
    std::vector<std::thread> threads;

    std::cout << name
              << ": sizeof(Counter) = " << sizeof(Counter)
              << ", alignof(Counter) = " << alignof(Counter)
              << "\n";

    auto start = Clock::now();
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (std::int64_t i = 0; i < ITERATIONS; ++i) {
                counters[t].value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    auto end = Clock::now();

    std::int64_t total = 0;
    for (const auto& counter : counters) {
        total += counter.value.load(std::memory_order_relaxed);
    }

    std::chrono::duration<double> elapsed = end - start;
    std::cout << name << " total = " << total
              << " , elapsed = " << elapsed.count() << " s\n";
}

int main() {
    run_benchmark<CompactCounter>("Compact counters");
    run_benchmark<PaddedCounter>("Padded counters");
    return 0;
}