// main.cpp
#include <iostream>
#include <thread>
#include <vector>
#include "spinlock.h" // or "minimal_spinlock.h"

int main() {
    SpinLock lock;
    constexpr int threads = 8;
    constexpr int iters = 200'000;
    long long counter = 0;

    auto worker = [&]{
        for (int i = 0; i < iters; ++i) {
            SpinGuard g(lock);
            ++counter; // critical section
        }
    };

    std::vector<std::thread> ts;
    for (int i = 0; i < threads; ++i) ts.emplace_back(worker);
    for (auto& t : ts) t.join();

    std::cout << "expected: " << 1LL * threads * iters
              << ", got: "    << counter << "\n";
}