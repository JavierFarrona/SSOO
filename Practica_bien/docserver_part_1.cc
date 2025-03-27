// docserver.h
#ifndef DOCSERVER_H
#define DOCSERVER_H

#include <string>

void print_help();
void verbose_log(const std::string &message, bool verbose);
std::string read_file(const std::string &file_path, bool &error, int &errno_val);

#endif // DOCSERVER_H

// docserver.cc
#include "docserver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>

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

// main.cc
#include "docserver.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    bool verbose = false;
    std::string file_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else {
            file_path = arg;
        }
    }

    if (file_path.empty()) {
        std::cerr << "Error: No file specified.\n";
        print_help();
        return 1;
    }

    verbose_log("Opening file: " + file_path, verbose);

    bool error = false;
    int errno_val = 0;
    std::string content = read_file(file_path, error, errno_val);

    if (error) {
        if (errno_val == EACCES) {
            std::cerr << "403 Forbidden\n";
        } else if (errno_val == ENOENT) {
            std::cerr << "404 Not Found\n";
        } else {
            std::cerr << "Error: " << std::strerror(errno_val) << "\n";
        }
        return 1;
    }

    std::cout << "Content-Length: " << content.size() << "\n\n";
    std::cout << content;

    verbose_log("File read successfully", verbose);

    return 0;
}