#include <cassert>
#include <thread>
#include <vector>
#include "../lab/spsc_ring.hpp"

int main() {
    constexpr int N = 100'000;
    lab::SpscRing<int, 1 << 12> q;
    std::atomic<bool> go{false};
    std::vector<int> out; out.reserve(N);

    std::thread prod([&]{
        while (!go.load(std::memory_order_acquire)) {}
        for (int i = 0; i < N; ++i) { while (!q.try_push(i)) {} }
    });
    std::thread cons([&]{
        while (!go.load(std::memory_order_acquire)) {}
        int v;
        for (int i = 0; i < N; ++i) { while (!q.try_pop(v)) {} out.push_back(v); }
    });

    go.store(true, std::memory_order_release);
    prod.join(); cons.join();

    assert(static_cast<int>(out.size()) == N);
    for (int i = 0; i < N; ++i) assert(out[i] == i);
    return 0;

}