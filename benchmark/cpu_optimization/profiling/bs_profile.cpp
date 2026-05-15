#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>

using Clock = std::chrono::steady_clock;

struct Params {
    double S;
    double K;
    double r;
    double sigma;
    double T;
};

__attribute__((noinline))
double normal_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

__attribute__((noinline))
double bs_call_price(const Params& p) {
    double sqrtT = std::sqrt(p.T);
    double d1 = (std::log(p.S / p.K) + (p.r + 0.5 * p.sigma * p.sigma) * p.T) / (p.sigma * sqrtT);
    double d2 = d1 - p.sigma * sqrtT;

    return p.S * normal_cdf(d1) - p.K * std::exp(-p.r * p.T) * normal_cdf(d2);
}

__attribute__((noinline))
double monte_carlo_call_price(const Params& p, std::int64_t paths, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> normal(0.0, 1.0);

    double drift = (p.r - 0.5 * p.sigma * p.sigma) * p.T;
    double diffusion_scale = p.sigma * std::sqrt(p.T);
    double discount = std::exp(-p.r * p.T);

    double payoff_sum = 0.0;

    for (std::int64_t i = 0; i < paths; ++i) {
        double z = normal(rng);
        double ST = p.S * std::exp(drift + diffusion_scale * z);
        double payoff = std::max(ST - p.K, 0.0);
        payoff_sum += payoff;
    }

    return discount * payoff_sum / static_cast<double>(paths);
}

int main() {
    Params p{
        .S = 100.0,
        .K = 100.0,
        .r = 0.05,
        .sigma = 0.2,
        .T = 1.0
    };

    constexpr std::int64_t bs_repeats = 50'000'000;
    constexpr std::int64_t mc_paths = 20'000'000;

    volatile double sink = 0.0;

    auto start = Clock::now();

    for (std::int64_t i = 0; i < bs_repeats; ++i) {
        sink += bs_call_price(p);
    }

    double mc_price = monte_carlo_call_price(p, mc_paths, 42);
    sink += mc_price;

    auto end = Clock::now();

    std::chrono::duration<double> elapsed = end - start;

    std::cout << "sink = " << sink << "\n";
    std::cout << "mc_price = " << mc_price << "\n";
    std::cout << "elapsed = " << elapsed.count() << " s\n";

    return 0;
}