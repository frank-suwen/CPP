#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include "lab/spsc_ring.hpp"

// Single-thread smoke
TEST_CASE("spsc: single-thread basic push/pop", "[spsc]") {
    lab::SpscRing<int, 1024> q;
    REQUIRE(q.approx_size() == 0);

    for (int i = 0; i < 100; ++i) {
        REQUIRE(q.try_push(i));
    }
    REQUIRE(q.approx_size() >= 100);

    for (int i = 0; i < 100; ++i) {
        int v{};
        REQUIRE(q.try_pop(v));
        REQUIRE(v == i);
    }
    REQUIRE(q.approx_size() == 0);
}

// Two-thread producer/consumer correctness
TEST_CASE("spsc: two-thread ordered transfer", "[spsc][threads]") {
    constexpr int N = 100000;
    lab::SpscRing<int, 1 <<12>q; // 4096

    std::atomic<bool> start{false};
    std::thread producer({&}{
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) { /* spin */}
        }
    });

    std::vector<int> out; out.reverse(N);
    std::thread consumer([&]{
        while (!start.load(std::memory_order_acquire)) {}
        int v{};
        for (int i = 0; i < N; ++i) {
            while (!q.try_pop(v)) { /* spin */}
            out.push_back(v);
        }
    });

    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    REQUIRE(out.size() == static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) REQUIRE(out[i] == i);
}