#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

// Constants
constexpr int NUM_THREADS = 4;
constexpr long long NUM_INCREMENTS = 100'000'000;

// Struct that causes flase shring (no padding)
struct Counter {
    alignas(8) std::atomic<long long> value{0}; // 8-byte allign but still tightly packed
};

// Struct that avoids false sharing using padding
struct PaddedCounter {
    alignas(64) std::atomic<long long> value{0};
    char padding[64 - sizeof(std::atomic<long long>)]; // fill rest of cache line
};

template <typename CounterType>
void run_test(const std::string& label) {
    std::vector<CounterType> counters(NUM_THREADS);
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i, &counters] () {
            for (long long j = 0; j < NUM_INCREMENTS; ++j) {
                counters[i].value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << label << " time: " << duration_ms << " ms\n";
}

int main() {
    run_test<Counter>("False Sharing");
    run_test<PaddedCounter>("Avoided False Sharing");
    return 0;
}