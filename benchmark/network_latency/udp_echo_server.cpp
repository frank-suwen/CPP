#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

} // namespace

int main() {
    constexpr int PORT = 9000;
    constexpr std::size_t BUFFER_SIZE = 2048;
    constexpr int MAX_EVENTS = 16;

    int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    if (!set_nonblocking(sockfd)) {
        std::cerr << "set_nonblocking failed: " << std::strerror(errno) << "\n";
        ::close(sockfd);
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

    int epfd = ::epoll_create1(0);
    if (epfd < 0) {
        std::cerr << "epoll_create1 failed: " << std::strerror(errno) << "\n";
        ::close(sockfd);
        return 1;
    }

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = sockfd;

    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) < 0) {
        std::cerr << "epoll_ctl failed: " << std::strerror(errno) << "\n";
        ::close(epfd);
        ::close(sockfd);
        return 1;
    }

    std::cout << "UDP epoll echo server listening on port " << PORT << "\n";

    epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    while (true) {
        int nready = ::epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "epoll_wait failed: " << std::strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < nready; ++i) {
            if (events[i].data.fd != sockfd) {
                continue;
            }

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
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }

                    std::cerr << "recvfrom failed: " << std::strerror(errno) << "\n";
                    break;
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
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // In a production system we would queue unsent messages.
                        // For this lab, we just drop if send buffer is full
                        break;
                    }

                    std::cerr << "sendto failed: " << std::strerror(errno) << "\n";
                    break;
                }
            }
        }
    }

    ::close(epfd);
    ::close(sockfd);
    return 0;
}