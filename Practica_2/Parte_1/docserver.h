#ifndef DOCSERVER_H
#define DOCSERVER_H

#include <string>

// Estructura para almacenar argumentos de línea de comandos
struct Args {
    bool verbose = false;
    std::string file_path;
};

// Funciones
Args parse_args(int argc, char* argv[]);
std::string read_all(const std::string& path);
void send_response(const std::string& header, const std::string& body = "");

#endif // DOCSERVER_H
