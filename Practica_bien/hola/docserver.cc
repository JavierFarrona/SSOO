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
#include <sys/wait.h>
#include <fcntl.h>

void print_help() {
    std::cout << "Usage: docserver [-v | --verbose] [-h | --help] [-p <port>] [-b <base_dir>]\n";
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

std::string process_request(const std::string &request, const std::string &base_dir, bool &error) {
    if (request.empty() || request.substr(0, 3) != "GET") {
        error = true;
        return "400 Bad Request\n";
    }

    std::string file_path = base_dir + request.substr(4, request.find(' ', 4) - 4);
    bool read_error = false;
    int errno_val = 0;
    std::string content = read_file(file_path, read_error, errno_val);

    if (read_error) {
        error = true;
        return (errno_val == EACCES) ? "403 Forbidden\n" : "404 Not Found\n";
    }

    std::ostringstream response;
    response << "Content-Length: " << content.size() << "\n\n";
    response << content;
    return response.str();
}

std::string execute_dynamic_content(const std::string &program_path, const std::string &base_dir) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        return "500 Internal Server Error\n";
    }

    pid_t pid = fork();
    if (pid == -1) {
        return "500 Internal Server Error\n";
    }

    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        std::string full_path = base_dir + "/bin/" + program_path;
        execl(full_path.c_str(), program_path.c_str(), nullptr);
        exit(1);
    } else {
        close(pipe_fd[1]);
        char buffer[1024];
        std::ostringstream output;
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
            output.write(buffer, bytes_read);
        }
        close(pipe_fd[0]);
        waitpid(pid, nullptr, 0);
        return output.str();
    }
}

void start_server(uint16_t port, const std::string &base_dir, bool verbose) {
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

        char buffer[1024];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0';
        std::string request(buffer);

        bool error = false;
        std::string response;
        if (request.find("/bin/") == 4) {
            response = execute_dynamic_content(request.substr(9), base_dir);
        } else {
            response = process_request(request, base_dir, error);
        }

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }

    close(server_fd);
}

