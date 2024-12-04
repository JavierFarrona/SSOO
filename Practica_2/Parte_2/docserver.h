#ifndef DOCSERVER_H
#define DOCSERVER_H

#include <string>
#include <cstdint>

// Funciones del servidor
int make_socket(uint16_t port);
void accept_connections(int server_sock);
void handle_client(int client_sock);
void send_response(const std::string& header, const std::string& body = "");

#endif // DOCSERVER_H
