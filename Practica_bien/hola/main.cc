// main.cc
#include "docserver.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char *argv[]) {
    bool verbose = false;
    uint16_t port = 8080;
    std::string base_dir = "./";

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
        } else if (arg == "-b" || arg == "--base") {
            if (i + 1 < argc) {
                base_dir = argv[++i];
            } else {
                std::cerr << "Error: Base directory not specified.\n";
                return 1;
            }
        }
    }

    verbose_log("Starting server...", verbose);
    start_server(port, base_dir, verbose);

    return 0;
}
