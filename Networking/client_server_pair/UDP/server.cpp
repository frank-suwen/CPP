// server.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>     // close()
#include <arpa/inet.h>  // socket, bind, recvfrom, sendto, htons, inet_ntop

int main() {
    // 1) Create UDP socket (SOCK_DGRAM)
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    // 2) Bind to 0.0.0.0:8081 (all local interfaces)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return 1;
    }
    std::cout << "UDP server listening on port 8081...\n";

    // 3) Receive datagrams in a loop
    while (true) {
        char buf[1500]; // ~MTU size is typical; 1500 fits most Ethernet frames
        sockaddr_in peer{};
        socklen_t peer_len = sizeof(peer);

        // recvfrom fills peer with sender's address
        ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0, (sockaddr*)&peer, &peer_len);
        if (n < 0) { perror("recvfrom"); continue; }

        buf[n] = '\n'; // make it printable
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        std::cout << "Got " << n << " bytes form " << ip
                  << ":" << ntohs(peer.sin_port)
                  << " -> \"" << buf << "\"\n";
        
        // 4) Echo back to the same peer (stateless: no "connection")
        ssize_t sent = sendto(fd, buf, n, 0, (sockaddr*)&peer, peer_len);
        if (sent < 0) perror("sendto");
    }

    // (Unreachable in this demo)
    close(fd);
    return 0;
}