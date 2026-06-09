#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    constexpr int PORT = 9000;

    int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
        std::cerr << "inet_pton failed\n";
        ::close(sockfd);
        return 1;
    }

    const char* message = "hello udp";

    ssize_t sent = ::sendto(
        sockfd,
        message,
        std::strlen(message),
        0,
        reinterpret_cast<sockaddr*>(&server_addr),
        sizeof(server_addr)
    );

    if (sent < 0) {
        std::cerr << "sendto failed: " << std::strerror(errno) << "\n";
        ::close(sockfd);
        return 1;
    }

    char buffer[2048];

    ssize_t n = ::recvfrom(
        sockfd,
        buffer,
        sizeof(buffer) - 1,
        0,
        nullptr,
        nullptr
    );

    if (n < 0) {
        std::cerr << "recvfrom failed: " << std::strerror(errno) << "\n";
        ::close(sockfd);
        return 1;
    }

    buffer[n] = '\0';
    std::cout << "received echo: " << buffer << "\n";

    ::close(sockfd);
    return 0;
}