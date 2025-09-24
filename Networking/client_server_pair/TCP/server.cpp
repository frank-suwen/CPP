// server.cpp
#include <iostream>
#include <cstring>        // memset
#include <unistd.h>       // close
#include <arpa/inet.h>    // socket, bind, listen, accept

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);    // 1. TCP socket
    if (server_fd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);                        // 2. Port 8080
    addr.sin_addr.s_addr = INADDR_ANY;                  // Bind to all interfaces
    
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(server_fd, 1) < 0) {                     // 3. Queue length 1
        perror("listen"); return 1;
    }

    std::cout << "Server listening on port 8080...\n";
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) { perror("accept"); return 1; }

    char buffer[1024] = {0};
    ssize_t n = read(client_fd, buffer, sizeof(buffer));
    if (n > 0) {
        std::cout << "Received: " << buffer << "\n";
        std::string reply = "Hello from server!";
        send(client_fd, reply.c_str(), reply.size(), 0);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}