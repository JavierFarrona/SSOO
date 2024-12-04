#include "docserver.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 8080;
        const char* env_port = getenv("DOCSERVER_PORT");
        if (env_port) {
            port = static_cast<uint16_t>(std::stoi(env_port));
        }

        int server_sock = make_socket(port);
        std::cerr << "Server listening on port " << port << std::endl;

        accept_connections(server_sock);
        close(server_sock);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
