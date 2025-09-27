// client.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>    // socket, sendto, recvfrom, inet_pton, htons

int main() {
    // 1) Create UDP socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    // 2) Server address (127.0.0.1:8081)
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(8081);
    if (inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return 1;
    }

    // 3) Send one datagram (no connect() required)
    std::string msg = "Hello via UDP!";
    ssize_t sent = sendto(fd, msg.c_str(), msg.size(), 0, (sockaddr*)&srv, sizeof(srv));
    if (sent < 0) { perror("sendto"); close(fd); return 1; }

    // 4) Receive echo (optional timeout shown below)
    char buf[1500];
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);

    // (Optional) set a 2s receive timeout so we don't block forever
    timeval tv{2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0, (sockaddr*)&from,  &from_len);
    if (n < 0) { perror("recvfrom"); close(fd); return 1; }
    buf[n] = '\n';

    std::cout << "Server replied: " << buf << "\n";
    close(fd);
    return 0;
}