#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr std::size_t N = 100'000'000;

std::int64_t branch_version(const std::vector<int>& data) {
    std::int64_t sum = 0;

    for (int x : data) {
        if (x >= 128) {
            sum += x;
        }
    }

    return sum;
}

std::int64_t branchless_version(const std::vector<int>& data) {
    std::int64_t sum = 0;

    for (int x : data) {
        sum += (x >= 128) * x;
    }

    return sum;
}

void run_case(const std::string& name, const std::vector<int>& data) {
    std::cout << "Case: " << name << "\n";

    {
        auto start = Clock::now();
        volatile std::int64_t sum = branch_version(data);
        auto end = Clock::now();

        std::chrono::duration<double> elapsed = end - start;

        std::cout << " Branch version:\n";
        std::cout << " sum = " << sum << "\n";
        std::cout << " elapsed = " << elapsed.count() << " s\n";
    }

    {
        auto start = Clock::now();
        volatile std::int64_t sum = branchless_version(data);
        auto end = Clock::now();

        std::chrono::duration<double> elapsed = end - start;

        std::cout << " Branchless version:\n";
        std::cout << " sum = " << sum << "\n";
        std::cout << " elapsed = " << elapsed.count() << "s\n";
    }

    std::cout << "\n";
}

int main() {
    std::vector<int> random_data;
    random_data.reserve(N);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);

    for (std::size_t i = 0; i < N; ++i) {
        random_data.push_back(dist(rng));
    }

    std::vector<int> sorted_data = random_data;
    std::sort(sorted_data.begin(), sorted_data.end());

    run_case("random / unpredictable branch", random_data);
    run_case("sorted / predictable branch", sorted_data);

    return 0;
}