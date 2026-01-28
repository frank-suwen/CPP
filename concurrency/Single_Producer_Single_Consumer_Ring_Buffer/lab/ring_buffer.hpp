#pragma once
#include <cstdint>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include "spsc_ring.hpp"

namespace lab {

// Concrete instantiation for demos/bench: 64-bit payload, 4096 slots.
using RingU64 = SpscRing<std::unit64_t, 1u << 12>;

struct BenchResult {
    long long times_ns{};
    int       transfers{};
    double    ns_per{};
    double    mega_transfers_per_s{};
};

// Non-template bench helper so we can compile it into a .cpp TU.
BenchResult run_transfer_bench(int N);

} // namespace lab