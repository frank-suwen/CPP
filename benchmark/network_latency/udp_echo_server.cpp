#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    constexpr int PORT = 9000;
    constexpr std::size_t BUFFER_SIZE = 2048;

    int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (::bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "bind failed: " << std::strerror(errno) << "\n";
        ::close(sockfd);
        return 1;
    }

    std::cout << "UDP echo server listening on port " << PORT << "\n";

    char buffer[BUFFER_SIZE];

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = ::recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE,
            0,
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_len
        );

        if (n < 0) {
            std::cerr << "recvfrom failed: " << std::strerror(errno) << "\n";
            continue;
        }

        ssize_t sent = ::sendto(
            sockfd,
            buffer,
            static_cast<std::size_t>(n),
            0,
            reinterpret_cast<sockaddr*>(&client_addr),
            client_len
        );

        if (sent < 0) {
            std::cerr << "sendto failed: " << std::strerror(errno) << "\n";
        }
    }

    ::close(sockfd);
    return 0;
}