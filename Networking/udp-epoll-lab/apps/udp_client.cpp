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
#include <unordered_map>
#include <vector>

static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog
        << "[--addr <ip>] [--port <port>] [--n <count>] [--payload <bytes>] [--window <W>]\n"
        << "Defaults: --addr 127.0.0.1 --port 9000 --n 100000 --payload 64 --window 64\n";
}

static uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static uint64_t percentile_ns(std::vector<uint64_t>& v, double p) {
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
    int window = 64;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--addr" && i + 1 < argc) addr_s = argv[++i];
        else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--n" && i + 1 < argc) n = std::atoi(argv[++i]);
        else if (a == "--payload" && i + 1 < argc) payload = std::atoi(argv[++i]);
        else if (a == "--window" && i + 1 < argc) window = std::atoi(argv[++i]);
        else if (a == "--help" || a == "-h")  { usage(argv[0]); return 0; }
        else { std::cerr << "Unknown arg: " << a << "\n"; usage(argv[0]); return 2; }
    }

    if (payload < 8) {
        std::cerr << "--payload must be >= 8 (we store a uint64 seq)\n";
        return 2;
    }

    if (window <= 0) {
        std::cerr << "--window must be > 0\n";
        return 2;
    }
    if (window > n) window = n; // clamp for small n

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

    // UDP connect(): sets default peer & filters incoming packets to that peer
    if (::connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != 0) {
        std::perror("connect");
        ::close(fd);
        return 1;
    }

    std::vector<char> buf(static_cast<size_t>(payload), 0);
    std::vector<char> rx(static_cast<size_t>(payload), 0);

    // We record send timestamps per sequence number.
    // Since we're only ever "window" in-flight, this stays small.
    std::unordered_map<uint64_t, uint64_t> sent_ns;
    sent_ns.reserve(static_cast<size_t>(window) * 2);

    std::vector<uint64_t> rtts;
    rtts.reserve(static_cast<size_t>(n));

    // ---- Warmup (still stop-and-wait; keeps it simple) ----
    const int warm = std::min(1000, n / 10);
    for (int i = 0; i < warm; ++i) {
        uint64_t seq = static_cast<uint64_t>(i);
        std::memcpy(buf.data(), &seq, sizeof(seq));
        (void)::send(fd, buf.data(), buf.size(), 0);
        (void)::recv(fd, rx.data(), rx.size(), 0);
    }

    // ---- Pipelined section ----
    int sent = 0;
    int received = 0;

    while (received < n) {
        // 1) Fill the window: send until we have 'window' outstanding.
        while (sent < n && (sent - received) < window) {
            uint64_t seq = static_cast<uint64_t>(sent);
            std::memcpy(buf.data(), &seq, sizeof(seq));

            uint64_t t0 = now_ns();
            ssize_t s = ::send(fd, buf.data(), buf.size(), 0);
            if (s != static_cast<ssize_t>(buf.size())) {
                std::perror("send");
                ::close(fd);
                return 1;
            }

            sent_ns.emplace(seq, t0);
            ++sent;
        }

        // 2) Drain replies: receive one (blocking) and compute RTT.
        // (We keep blocking recv for now to keep the lab minimal and stable.)
        ssize_t r = ::recv(fd, rx.data(), rx.size(), 0);
        uint64_t t1 = now_ns();
        if (r != static_cast<ssize_t>(rx.size())) {
            std::perror("recv");
            ::close(fd);
            return 1;
        }

        uint64_t got = 0;
        std::memcpy(&got, rx.data(), sizeof(got));

        auto it = sent_ns.find(got);
        if (it == sent_ns.end()) {
            std::cerr << "unexpected seq in reply: " << got << "\n";
            ::close(fd);
            return 1;
        }

        rtts.push_back(t1 - it->second);
        sent_ns.erase(it);
        ++received;
    }

    ::close(fd);

    uint64_t p50 = percentile_ns(rtts, 0.50);
    uint64_t p99 = percentile_ns(rtts, 0.99);

    std::cout << "Environment: localhost on VM\n";
    std::cout << "Messages: " << rtts.size() 
              << " payload: " << payload 
              << " window: " << window << "\n";
    std::cout << "p50 RTT: " << (p50 / 1000.0) << " us\n";
    std::cout << "p99 RTT: " << (p99 / 1000.0) << " us\n";
    return 0;
}