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

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>
#include <expected>
#include <iomanip>
#include <string_view>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <sys/mman.h> 
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <format>


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

// Variable global
bool flag_v = false;

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
enum class error_parse_args{
  argumento_faltante,
  opcion_desconocida,
  // ...
};

/**
 * @brief Estructura que representa las opciones de un programa.
 */
struct opciones_programa {
  bool flag_h = false;
  std::string output_filename;
  // ...
  std::vector<std::string> additional_args;
};

/**
 * @brief Estructura que representa un error al ejecutar un programa.
 */
struct execute_program_error {
  int exit_code;
  int error_code;
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
  std::string_view sv_; // std::string_view que almacena el archivo mapeado
};

/**
 * @brief Parsea los argumentos de la línea de comandos.
 * @param argc Número de argumentos.
 * @param argv Argumentos.
 */
std::expected<opciones_programa, error_parse_args> parse_args(int argc, char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  opciones_programa options;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.flag_h = true;
    } else if (*it == "-v" || *it == "--verbose") {
      flag_v = true;
    } else if (it->starts_with("-")) {
      options.additional_args.push_back(std::string(*it));
    // Comprobar que el argumento es un archivo
    } else if (std::filesystem::exists(*it)) {
      options.output_filename = *it;
    } else {
      return std::unexpected(error_parse_args::opcion_desconocida);
    }
  }

  return options;
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
  print_verbose("Open -> Archivo \"" + path + "\" abierto correctamente");

  off_t length = lseek(fd, 0, SEEK_END);
  if (length < 0) {
    // Error al obtener el tamaño del archivo...
    print_verbose("Error al obtener el tamaño del archivo");
    close(fd);
    return std::unexpected(errno);
  }
  print_verbose("Lseek -> Tamaño del archivo \"" + path + "\" obtenido correctamente");

  void* mem = mmap(NULL, static_cast<size_t>(length), PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  print_verbose("Close -> Archivo \"" + path + "\" cerrado correctamente");
  if (mem == MAP_FAILED) {
    // Error al mapear el archivo...
    print_verbose("Mmap: error al mapear el archivo \"" + path + "\" (errno: " + std::to_string(errno) + ")");
    return std::unexpected(errno);
  }
  print_verbose("Mmap -> Archivo \"" + path + "\" mapeado correctamente");

  return SafeMap(std::string_view(static_cast<char*>(mem), static_cast<size_t>(length)));
}

/**
 * @brief Envia la respuesta al cliente.
 * @param cabecera_ Cabecera de la respuesta.
 * @param cuerpo_ Cuerpo de la respuesta.
 */
void send_response(std::string_view cabecera_, std::string_view cuerpo_ = {}) {
  std::cout << cabecera_ << "\n";
  std::cout << cuerpo_ << "\n";
}

void Usage(char* argv[]) {
  std::cout << "Usage: " << argv[0] << " [-v | --verbose] [-h | --help]" 
    << " ARCHIVO\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help    Show this help mensaje\n";
  std::cout << "  -v, --verbose Enable verbose mode\n";
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
      case error_parse_args::argumento_faltante:
        std::cerr << "Error: missing argument\n";
        break;
      case error_parse_args::opcion_desconocida:
        std::cerr << "Error: unknown option\n";
        break;
      default:
        std::cerr << "Error: unknown error\n";
        break;
    }
    return 1;
  }

  const auto& options = result.value();

  if (options.flag_h) {
    Usage(argv);
    return 0;
  }

  if (options.output_filename.empty()) {
    std::cerr << "Error: missing output filename\n";
    return 1;
  }

  print_verbose("Read_all -> Leyendo archivo \"" + options.output_filename + "\"");
  auto file_content = read_all(options.output_filename);

  if (!file_content) {
    switch (file_content.error()) {
      case EACCES:
        std::cerr << "403 Forbidden\n";
        break;
      case ENOENT:
        std::cerr << "404 Not Found\n";
        break;
      default:
        std::cerr << "418 I'm a teapot (unknown error)\n";
        break;
      }
    return 1;
  }

  SafeMap safe_map = std::move(file_content.value());
  size_t size = safe_map.get().size();
  std::string cabecera_ = std::format("Content-Length: {}\r\n", size);
  std::string_view cuerpo_ = safe_map.get();
  send_response(cabecera_, cuerpo_);

  return 0;
}