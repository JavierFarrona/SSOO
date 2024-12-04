#include "docserver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

Args parse_args(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: docserver [-v|--verbose] [-h|--help] <file>\n";
            exit(0);
        } else if (arg == "-v" || arg == "--verbose") {
            args.verbose = true;
        } else if (arg[0] == '-') {
            throw std::invalid_argument("Unknown option: " + arg);
        } else {
            args.file_path = arg;
        }
    }
    if (args.file_path.empty()) {
        throw std::invalid_argument("No file specified.");
    }
    return args;
}

std::string read_all(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("403 Forbidden");
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void send_response(const std::string& header, const std::string& body) {
    std::cout << header << "\n\n";
    if (!body.empty()) {
        std::cout << body;
    }
}
