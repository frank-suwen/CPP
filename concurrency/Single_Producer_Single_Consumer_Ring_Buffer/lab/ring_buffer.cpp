#include "ring_buffer.hpp"

namespace lab {

BenchResult run_transfer_bench(int N) {
    RingU64 q;
    std::atomic<bool> start{false};

    using spsc_clock = std::chrono::steady_clock;
    auto t0 = spsc_clock::now();

    std::thread prod([&]{
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(static_cast<std::uint64_t>(i))) {}
        }
    });

    std::thread cons([&]{
        while (!start.load(std::memory_order_acquire)) {}
        std::uint64_t v;
        for (int i = 0; i < N; ++i) {
            while (!q.try_pop(v)) {}
        }
    });

    start.store(true, std::memory_order_release);
    prod.join();
    cons.join();

    auto t1 = spsc_clock::now();
    const long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    BenchResult r;
    r.times_ns = ns;
    r.transfers = N;
    r.ns_per = static_cast<double>(ns) / static_cast<double>(N);
    r.mega_transfers_per_s = 1e3 / r.ns_per; // ns -> us -> MT/s
    return r;
}

} // namespace lab