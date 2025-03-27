/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Asignatura: Sistemas Operativos
 * Curso: 2º
 * Autor: Javier Farrona Cabrera
 * Correo: alu0101541983@ull.edu.es
 * Fecha: 19 Nov 2024
 * Archivo: docserver.cc
 * Referencias:
 *     Enunciado de la práctica
 */

#include <fcntl.h>
#include <netinet/ip.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

/**
 * En la terminal: socat STDIO TCP:127.0.0.1:8080
 */

/**
 * Compilar con:
 * g++ -std=c++23 -Wall -Wextra -Werror -Wpedantic \
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast \
    -Wcast-align -Wunused -Woverloaded-virtual \
    -Wconversion -Wsign-conversion -Wnull-dereference \
    -Wdouble-promotion -Wformat=2 -Wmisleading-indentation \
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
    -Wuseless-cast -fsanitize=address,undefined,leak -o docserver docserver.cc
 */

// Variables globales
bool flag_v = false;
uint16_t port = 8080;
bool flag_base_dir = false;
std::string base_dir;

/**
 * @brief Imprime un mensaje en modo verbose.
 * @param mensaje Mensaje.
 */
void print_verbose(std::string mensaje) {
  if (flag_v) {
    std::cout << mensaje << "\n";
  }
}

/**
 * @brief Enumeración que representa los errores al parsear los argumentos.
 */
enum class parse_args_errors {
  argumento_faltante,
  opcion_desconocida,
  no_indica_puerto,
  puerto_no_usable,
  // ...
};

/**
 * @brief Estructura que representa las opciones de un programa.
 */
struct program_options {
  bool flag_h = false;
  std::string output_filename;
  int port = 0;
  std::vector<std::string> additional_args;
  std::string base_directory;
};

/**
 * @brief Estructura que representa un error al ejecutar un programa.
 */
struct execute_program_error {
  int codigo_salida;
  int codigo_error;
};

struct exec_environment {
  std::string cwd;
  std::vector<std::string> env;
};

/**
 * @brief Clase que mapea un archivo en memoria de forma segura.
 */
class SafeMap {
 public:
  // Constructor por defecto
  SafeMap() = default;
  // Constructor que recibe un std::string_view
  SafeMap(std::string_view sv) : sv_(sv) {}
  // Destructor llama a munmap con la dirrecion y el tamaño
  ~SafeMap() {
    if (sv_.data() != nullptr) {
      munmap(const_cast<char*>(sv_.data()), sv_.size());
    }
  }

  // Método para obtener el std::string_view
  std::string_view get() const { return sv_; }

  // Prohibir la copia
  SafeMap(const SafeMap&) = delete;
  SafeMap& operator=(const SafeMap&) = delete;

  // Permitir el movimiento
  SafeMap(SafeMap&& other) : sv_(other.sv_) { other.sv_ = std::string_view(); }

  SafeMap& operator=(SafeMap&& other) {
    if (this != &other) {
      if (sv_.data() != nullptr) {
        munmap(const_cast<char*>(sv_.data()), sv_.size());
      }
      sv_ = other.sv_;
      other.sv_ = std::string_view();
    }
    return *this;
  }

 private:
  std::string_view sv_;  // std::string_view que almacena el archivo mapeado
};

class SafeFD {
 public:
  // Constructor
  explicit SafeFD(int fd) noexcept : fd_(fd) {}
  explicit SafeFD() noexcept : fd_(-1) {}

  SafeFD(const SafeFD&) = delete;
  SafeFD& operator=(const SafeFD&) = delete;

  SafeFD(SafeFD&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

  SafeFD& operator=(SafeFD&& other) noexcept {
    if (this != &other && fd_ != other.fd_) {
      // Cerrar el descriptor de archivo actual
      close(fd_);

      // Mover el descriptor de archivo de 'other' a este objeto
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  ~SafeFD() noexcept {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }

  [[nodiscard]] int get() const noexcept { return fd_; }

 private:
  int fd_;
};

/**
 * @brief Parsea los argumentos de la línea de comandos.
 * @param argc Número de argumentos.
 * @param argv Argumentos.
 */
std::expected<program_options, parse_args_errors> parse_args(int argc,
                                                             char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.flag_h = true;
    } else if (*it == "-v" || *it == "--verbose") {
      flag_v = true;
    } else if (*it == "-p" || *it == "--port") {
      if (++it != end && !it->starts_with("-")) {
        try {
          int port_number = std::stoi(std::string(*it));
          if (port_number < 1024 || port_number > 65535) {
            return std::unexpected(parse_args_errors::puerto_no_usable);
          }
          port = static_cast<uint16_t>(port_number);
        } catch (const std::invalid_argument&) {
          return std::unexpected(parse_args_errors::opcion_desconocida);
        }
      } else {
        return std::unexpected(parse_args_errors::no_indica_puerto);
      }
    } else if (*it == "-b" || *it == "--base") {
      flag_base_dir = true;
      if (++it != end && !it->starts_with("-")) {
        base_dir = *it;
      } else {
        return std::unexpected(parse_args_errors::argumento_faltante);
      }
    } else if (std::filesystem::exists(*it)) {
      options.output_filename = *it;
    } else if (it->starts_with("-") || it->starts_with("--")) {
      return std::unexpected(parse_args_errors::opcion_desconocida);
    } else {
      options.additional_args.push_back(std::string(*it));
    }
  }

  return options;
}

void Usage(char* argv[]) {
  std::cout << "Usage: " << argv[0] << " [-v | --verbose] [-h | --help]"
            << "[-p <puerto> | --port <puerto>] [-b <ruta> | --base <ruta>]\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help    Show this help mensaje\n";
  std::cout << "  -v, --verbose Enable verbose mode\n";
  std::cout << "  -p, --port    Set port number\n";
  std::cout << "  -b, --base    Set base directory\n";
}

/**
 * @brief Crear un socket y asignarle el puerto indicado
 */
std::expected<SafeFD, int> make_socket(uint16_t socket_port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    print_verbose("Error al crear el socket");
    return std::unexpected(errno);
  }
  print_verbose("Make_socket: Socket creado");

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  print_verbose("Setsockopt: Socket configurado");

  sockaddr_in local_address{};
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = htonl(INADDR_ANY);
  local_address.sin_port = htons(socket_port);

  int result = bind(fd, reinterpret_cast<sockaddr*>(&local_address),
                    sizeof(local_address));
  if (result < 0) {
    print_verbose("Error al enlazar el socket");
    close(fd);
    return std::unexpected(errno);
  }
  print_verbose("Bind: Socket enlazado");

  return SafeFD(fd);
}

/**
 *
 */
std::string getenv(const std::string& name) {
  char* value = getenv(name.c_str());
  if (value) {
    return std::string(value);
  } else {
    return std::string();
  }
}

/**
 * @brief Escuchar conexiones en el puerto indicado
 */
int listen_connection(const SafeFD& socket) {
  int result = listen(socket.get(), 10);
  if (result < 0) {
    print_verbose("Error al escuchar conexiones");
    return errno;
  }
  print_verbose("Listen: Escuchando conexiones");
  return 0;
}

std::expected<std::string, int> receive_request(const SafeFD& socket,
                                                size_t max_size) {
  std::string buffer(max_size, '\0');
  ssize_t result = recv(socket.get(), buffer.data(), buffer.size(), 0);
  if (result < 0) {
    print_verbose("Error al recibir la petición");
    return std::unexpected(errno);
  }
  print_verbose("Recv: Peticion recibida");
  return buffer;
}

/**
 * @brief Aceptar una conexión
 */
std::expected<SafeFD, int> accept_connection(const SafeFD& socket,
                                             sockaddr_in& client_addr) {
  socklen_t client_addr_len = sizeof(client_addr);
  int client_fd =
      accept(socket.get(), reinterpret_cast<sockaddr*>(&client_addr),
             &client_addr_len);
  if (client_fd < 0) {
    print_verbose("Error al aceptar la conexión");
    return std::unexpected(errno);
  }
  print_verbose("Accept: Conexion aceptada");
  return SafeFD(client_fd);
}

/**
 * @brief Lee el contenido de un archivo.
 * @param path Ruta del archivo.
 * @return Contenido del archivo.
 */
std::expected<SafeMap, int> read_all(const std::string& path) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    // Error al abrir el archivo...
    print_verbose("Error al abrir el archivo");
    return std::unexpected(errno);
  }
  print_verbose("Open: Archivo \"" + path + "\" abierto correctamente");

  off_t length = lseek(fd, 0, SEEK_END);
  if (length < 0) {
    // Error al obtener el tamaño del archivo...
    print_verbose("Error al obtener el tamaño del archivo");
    close(fd);
    return std::unexpected(errno);
  }

  void* mem =
      mmap(NULL, static_cast<size_t>(length), PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  print_verbose("Close: Archivo \"" + path + "\" cerrado correctamente");
  if (mem == MAP_FAILED) {
    // Error al mapear el archivo...
    print_verbose("Mmap: error al mapear el archivo \"" + path +
                  "\" (errno: " + std::to_string(errno) + ")");
    return std::unexpected(errno);
  }
  print_verbose("Mmap: archivo \"" + path + "\" mapeado correctamente");

  return SafeMap(
      std::string_view(static_cast<char*>(mem), static_cast<size_t>(length)));
}

/**
 * @brief Envia la respuesta al cliente.
 * @param header Cabecera de la respuesta.
 * @param body Cuerpo de la respuesta.
 */
void send_response(std::string_view header, std::string_view body = {}) {
  std::cout << header << "\n";
  std::cout << body << "\n\n";
}

int send_response(const SafeFD& socket, std::string_view header,
                  std::string_view body) {
  std::string response = std::string(header) + "\r\n" + std::string(body);
  ssize_t result = send(socket.get(), response.data(), response.size(), 0);
  if (result < 0) {
    print_verbose("Error al enviar la respuesta");
    return errno;
  }
  print_verbose("Send: Respuesta enviada");
  return 0;
}

/**
 * @brief Punto de entrada del programa.
 * @param argc Número de argumentos.
 * @param argv Argumentos.
 */
int main(int argc, char* argv[]) {
  auto result = parse_args(argc, argv);
  if (!result) {
    switch (result.error()) {
      case parse_args_errors::argumento_faltante:
        std::cerr << "Error: missing argument\n";
        break;
      case parse_args_errors::opcion_desconocida:
        std::cerr << "Error: unknown option\n";
        break;
      case parse_args_errors::no_indica_puerto:
        std::cerr << "Error: missing port\n";
        break;
      case parse_args_errors::puerto_no_usable:
        std::cerr << "Error: port out of bounds\n";
        break;
      default:
        std::cerr << "Error: unknown error\n";
        break;
    }
    return EXIT_FAILURE;
  }

  /*
  std::string env_port = getenv("DOCSERVER_PORT");
  std::string env_dir = getenv("DOCSERVER_DIR");

  if (!flag_p) {
    if (env_port.empty()) {
      port = std::stoi(env_port);
    }
  }

  if (!flag_base_dir) {
    if (env_dir.empty()) {
      char* buffer = getcwd(NULL, 0);
      base_dir = buffer;
      free(buffer);
    }
    else {
      base_dir = env_dir;
    }
  }
*/

  if (!flag_base_dir) {
    char* buffer = getcwd(NULL, 0);
    base_dir = buffer;
    free(buffer);
  }

  const auto& options = result.value();

  if (options.flag_h) {
    Usage(argv);
    return EXIT_SUCCESS;
  }

  auto socket = make_socket(port);
  if (!socket) {
    switch (socket.error()) {
      case ECONNRESET:
        std::cerr << "Error: connection reset by peer\n";
        break;
      default:
        std::cerr << "Error: unknown error\n";
        break;
    }
    return EXIT_FAILURE;
  }

  if (listen_connection(socket.value()) != 0) {
    switch (listen_connection(socket.value())) {
      case ECONNRESET:
        std::cerr << "Error: connection reset by peer\n";
        break;
      default:
        std::cerr << "Error: unknown error\n";
        break;
    }
    return EXIT_FAILURE;
  }

  while (true) {
    sockaddr_in client_addr;
    auto client = accept_connection(socket.value(), client_addr);
    if (!client) {
      switch (client.error()) {
        case ECONNRESET:
          std::cerr << "Error: connection reset by peer\n";
          break;
        default:
          std::cerr << "Error: unknown error\n";
          break;
      }
      return EXIT_FAILURE;
    }

    print_verbose("Recibiendo petición");
    auto request = receive_request(client.value(), 1024);

    if (!request) {
      switch (request.error()) {
        case ECONNRESET:
          send_response(client.value(), "400 Bad Request", "");
          std::cerr << "Error: connection reset by peer\n";
          break;
        default:
          std::cerr << "Error: unknown error\n";
          break;
      }
      return EXIT_FAILURE;
    } else {
      print_verbose("Petición recibida: " + request.value());

      std::istringstream iss(request.value());
      std::string get, output_filename;
      iss >> get >> output_filename;

      // Errores
      if (get != "GET") {
        send_response(client.value(), "400 Bad Request", "");
        std::cerr << "Error: method not allowed\n";
        return EXIT_FAILURE;
      }

      if (output_filename.empty()) {
        send_response(client.value(), "400 Bad Request", "");
        std::cerr << "Error: bad request\n";
        return EXIT_FAILURE;
      }

      if (output_filename.front() != '/' || output_filename.back() == '/') {
        send_response(client.value(), "400 Bad Request", "");
        std::cerr << "Error: bad request\n";
        return EXIT_FAILURE;
      }

      // Si get empieza por "/bin"
      if (output_filename.starts_with("/bin")) {
        std::cout << "Por hacer...\n";
      } else {
        auto file_content = read_all(base_dir + output_filename);

        if (!file_content) {
          switch (file_content.error()) {
            case EACCES:
              std::cerr << "403 Forbidden\n";
              break;
            case ENOENT:
              std::cerr << "404 Not Found\n";
              break;
            default:
              std::cerr << "Error: unknown error\n";
              break;
          }
          return EXIT_FAILURE;
        }

        SafeMap safe_map = std::move(file_content.value());
        size_t size = safe_map.get().size();
        std::string header = std::format("Content-Length: {}\r\n", size);
        std::string_view body = safe_map.get();

        if (send_response(client.value(), header, body) != 0) {
          switch (send_response(client.value(), header, body)) {
            case ECONNRESET:
              std::cerr << "Error: connection reset by peer\n";
              break;
            default:
              std::cerr << "Error: unknown error\n";
              break;
          }
          return EXIT_FAILURE;
        }
      }
    }

    // Cerrar la conexión
    close(client.value().get());
    print_verbose("Conexión cerrada");
  }

  return EXIT_SUCCESS;
}