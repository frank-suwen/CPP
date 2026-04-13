#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <list>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

template <typename Container>
std::int64_t iterate_and_sum(const Container& c, int repeats) {
    std::int64_t total = 0;
    for (int r = 0; r < repeats; ++r) {
        for (int x : c) {
            total += x;
        }
    }
    return total;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./container_iter_bench [vector|deque|list]\n";
        return 1;
    }

    std::string mode = argv[1];
    constexpr std::size_t N = 50'000'000;
    constexpr int repeats = 5;

    if (mode == "vector") {
        std::vector<int> vec;
        vec.reserve(N);
        for (std::size_t i = 0; i < N; ++i) {
            vec.push_back(static_cast<int>(i % 100));
        }

        volatile std::int64_t warmup = iterate_and_sum(vec, 1);
        (void)warmup;

        auto start = Clock::now();
        volatile std::int64_t sum = iterate_and_sum(vec, repeats);
        auto end = Clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "vector sum = " << sum
                  << ", elapsed = " << elapsed.count() << " s\n";
    } else if (mode == "deque") {
        std::deque<int> deq;
        for (size_t i = 0; i < N; ++i) {
            deq.push_back(static_cast<int>(i % 100));
        }

        volatile std::int64_t warmup = iterate_and_sum(deq, 1);
        (void)warmup;

        auto start = Clock::now();
        volatile std::int64_t sum = iterate_and_sum(deq, repeats);
        auto end = Clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "deque sum = " << sum
                  << ",  elapsed = " << elapsed.count() << " s\n";
    } else if (mode == "list") {
        std::list<int> lst;
        for (std::size_t i = 0; i < N; ++i) {
            lst.push_back(static_cast<double>(i % 100));
        }

        volatile std::int64_t warmup = iterate_and_sum(lst, 1);
        (void)warmup;

        auto start = Clock::now();
        volatile std::int64_t sum = iterate_and_sum(lst, repeats);
        auto end = Clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "list sum = " << sum
                  << ",   elapsed = " << elapsed.count() << " s\n";
    } else {
        std::cerr << "Invalid mode. Use: vector, deque, or list\n";
        return 1;
    }

    return 0;
}