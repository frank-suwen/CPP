#include "ring_buffer.hpp"
#include <cstdlib>

int main(int argc, char** argv) {
    int N = 2'000'000;
    if (argc > 1) N = std::atoi(argv[1]);

    auto r = lab::run_transfer_bench(N);
    std::cout << "transfers=" << r.transfers << "\n"
              << "time_ns=" << r.time_ns << "\n"
              << "ns_per_transfer=" << r.ns_per << "\n"
              << "M_transfers_per_s=" << r.mega_transfers_per_s << "\n";
              
    return 0;
}