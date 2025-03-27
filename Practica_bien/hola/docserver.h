// docserver.h
#ifndef DOCSERVER_H
#define DOCSERVER_H

#include <string>
#include <cstdint>

void print_help();
void verbose_log(const std::string &message, bool verbose);
std::string read_file(const std::string &file_path, bool &error, int &errno_val);
void start_server(uint16_t port, const std::string &file_path, bool verbose);
std::string process_request(const std::string &request, const std::string &base_dir, bool &error);
std::string execute_dynamic_content(const std::string &program_path, const std::string &base_dir);

#endif // DOCSERVER_H
