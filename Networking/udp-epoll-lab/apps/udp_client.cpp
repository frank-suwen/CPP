#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog
        << "[--addr <ip>] [--port <port>] [--n <count>] [--payload <bytes>]\n"
        << "Defaults: --addr 127.0.0.1 --port 9000 --n 100000 --payload 64\n";
}

static uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static uint64_t percentile_ns(std::vector<uint64_t>& v, double p) {
    // p in [0, 1]
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t idx = static_cast<size_t>(p * (v.size() - 1));
    return v[idx];
}

int main(int argc, char** argv) {
    std::string addr_s = "127.0.0.1";
    int port = 9000;
    int n = 100000;
    int payload = 64;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--addr" && i + 1 < argc) addr_s = argv[++i];
        else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--n" && i + 1 < argc) n = std::atoi(argv[++i]);
        else if (a == "--payload" && i + 1 < argc) payload == std::atoi(argv[++i]);
        else if (a == "--help" || a == "-h")  { usage(argv[0]); return 0; }
        else { std::cerr << "Unknown arg: " << a << "\n"; usage(argv[0]); return 2; }
    }

    if (payload < 8) {
        std::cerr << "--payload must be >= 8 (we store a uint64 seq)\n";
        return 2;
    }

    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, addr_s.c_str(), &server.sin_addr) != 1) {
        std::cerr << "inet_pton failed for addr: " << addr_s << "\n";
        ::close(fd);
        return 1;
    }

    // Connect() on UDP is fine: it sets default peer and filters incoming packets.
    if (::connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != 0) {
        std::perror("connect");
        ::close(fd);
        return 1;
    }

    std::vector<char> buf(static_cast<size_t>(payload), 0);
    std::vector<char> rx(static_cat<size_t>(payload), 0);
    std::vector<uint64_t> rtts;
    rtts.reserve(static_cast<sizt_t>(n));

    // Warmup a bit to stabilize caches/scheduling
    const int warm = std::min(1000, n / 10);
    for (int i = 0; i < warm; ++i) {
        uint64_t seq = static_cast<uint64_t>(i);
        std::memcpy(buf.data(), &seq, sizeof(seq));
        (void)::send(fd, buf.data(), buf.size(), 0);
        (void)::recv(fd, rx.data(), rx.size(), 0);
    }

    for (int i = 0; i < n; ++i) {
        uint64_t seq = static_cast<uint64_t>(i);
        std::memcpy(buf.data(), &seq, sizeof(seq));

        uint64_t t0 = now_ns();
        ssize_t s = ::send(fd, buf.data(), buf.size(), 0);
        if (s != static_cast<ssize_t>(buf.size())) {
            std::perror("send");
            break;
        }

        ssize_t r = ::recv(fd, rx.data(), rx.size(), 0);
        uint64_t t1 = now_ns();
        if (r != static_cast<ssize_t>(rx.size())) {
            std::perror("recv");
            break;
        }

        uint64_t got = 0;
        std::memcpy(&got, rx.data(), sizeof(got));
        if (got != seq) {
            std::cerr << "sequence mismatch: got " << got << " expected " << seq << "\n";
            break;
        }

        rtts.push_back(t1 - t0);
    }

    ::close(fd);

    uint64_t p50 = percentile_ns(rtts, 0.50);
    uint64_t p99 = percentile_ns(rtts, 0.99);

    std::cout << "Environment: localhost on VM\n";
    std::cout << "Messages: " << rtts.size() << " payload: " << payload << " bytes\n";
    std::cout << "p50 RTT: " << (p50 / 1000.0) << " us\n";
    std::cout << "p99 RTT: " << (p99 / 1000.0) << " us\n";
    return 0;
}