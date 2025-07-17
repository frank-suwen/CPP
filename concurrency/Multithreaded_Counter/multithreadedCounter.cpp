#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::size_t counter = 0;
std::mutex m;

void increment(std::size_t times) {
    for (std::size_t i = 0; i < times; ++i) {
        std::lock_guard<std::mutex> lock(m);
        ++counter;
    }
}

int main() {
    constexpr std::size_t increments_per_thread = 100'000;
    constexpr std::size_t num_threads           = 8;

    // 1. Launch threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(increment, increments_per_thread);
    }

    // 2. Wait for them to finish (join)
    for (auto& t : threads) {
        t.join();
    }

    // 3. Check result
    std::cout << "Expected: "
              << increments_per_thread * num_threads
              << "\nActual: " << counter << '\n';
}