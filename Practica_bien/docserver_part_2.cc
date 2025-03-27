// docserver.h
#ifndef DOCSERVER_H
#define DOCSERVER_H

#include <string>
#include <cstdint>

void print_help();
void verbose_log(const std::string &message, bool verbose);
std::string read_file(const std::string &file_path, bool &error, int &errno_val);
void start_server(uint16_t port, const std::string &file_path, bool verbose);

#endif // DOCSERVER_H

// docserver.cc
#include "docserver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void print_help() {
    std::cout << "Usage: docserver [-v | --verbose] [-h | --help] <file>\n";
}

void verbose_log(const std::string &message, bool verbose) {
    if (verbose) {
        std::cerr << message << std::endl;
    }
}

std::string read_file(const std::string &file_path, bool &error, int &errno_val) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        error = true;
        errno_val = errno;
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void start_server(uint16_t port, const std::string &file_path, bool verbose) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error: " << std::strerror(errno) << "\n";
        exit(1);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error: " << std::strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Error: " << std::strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }

    verbose_log("Server started on port " + std::to_string(port), verbose);

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Error: " << std::strerror(errno) << "\n";
            continue;
        }

        verbose_log("Connection accepted", verbose);

        bool error = false;
        int errno_val = 0;
        std::string content = read_file(file_path, error, errno_val);

        if (error) {
            std::string response = (errno_val == EACCES) ? "403 Forbidden\n" : "404 Not Found\n";
            send(client_fd, response.c_str(), response.size(), 0);
        } else {
            std::ostringstream header;
            header << "Content-Length: " << content.size() << "\n\n";
            std::string response = header.str() + content;
            send(client_fd, response.c_str(), response.size(), 0);
        }

        close(client_fd);
    }

    close(server_fd);
}

// main.cc
#include "docserver.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char *argv[]) {
    bool verbose = false;
    uint16_t port = 8080;
    std::string file_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else {
                std::cerr << "Error: Port number not specified.\n";
                return 1;
            }
        } else {
            file_path = arg;
        }
    }

    if (file_path.empty()) {
        std::cerr << "Error: No file specified.\n";
        print_help();
        return 1;
    }

    verbose_log("Starting server...", verbose);
    start_server(port, file_path, verbose);

    return 0;
}