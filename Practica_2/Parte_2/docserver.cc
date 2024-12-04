#include "docserver.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

int make_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(sock);
        throw std::runtime_error("Bind failed");
    }
    if (listen(sock, 10) == -1) {
        close(sock);
        throw std::runtime_error("Listen failed");
    }
    return sock;
}

void accept_connections(int server_sock) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock == -1) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }
        handle_client(client_sock);
    }
}

void handle_client(int client_sock) {
    const std::string response = "Content-Length: 13\n\nHello, Client!";
    send(client_sock, response.c_str(), response.size(), 0);
    close(client_sock);
}

void send_response(const std::string& header, const std::string& body) {
    std::string response = header + "\n\n" + body;
    std::cout << response; // Optional logging
}
