// client.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Connect to localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton"); return 1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); return 1;
    }

    std::string msg = "Hello from client!";
    send(sock, msg.c_str(), msg.size(), 0);

    char buffer[1024] = {0};
    ssize_t n = read(sock, buffer, sizeof(buffer));
    if (n > 0) std::cout << "Server replied: " << buffer << "\n";

    close(sock);
    return 0;
}