#include "docserver.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Args args = parse_args(argc, argv);
        if (args.verbose) {
            std::cerr << "Reading file: " << args.file_path << std::endl;
        }
        std::string content = read_all(args.file_path);
        send_response("Content-Length: " + std::to_string(content.size()), content);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
