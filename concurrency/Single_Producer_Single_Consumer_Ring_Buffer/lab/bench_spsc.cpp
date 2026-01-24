#include "lab/spsc_ring.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using clock_t = std::chrono::steady_clock;

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    constexpr int N = 2'000'000;
    lab::SpscRing<int, 1 << 12> q;

    std::atomic<bool> start{false};
    auto t0 = clock_t::now();

    std::thread producer([&]{
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) {}
        }
    });

    std::thread consumer([&]{
        while (!start.load(std::memory_order_acquire)) {}
        int v;
        for (int i = 0; i < N; ++i) {
            while (!q.try_pop(v)) {}
        }
    });

    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    auto t1 = clock_t::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    double ns_per_transfer = static_cast<double>(ns) / N;

    std::cout << "transfers=" << N << "\n";
    std::cout << "time_ns=" << ns << "\n";
    std::cout << "ns_per_transfer=" << ns_per_transfer << "\n";
    std::cout << "M transfers/s=" << (1e3 / ns_per_transfer) << "\n"; // since ns->us
    return 0;
}