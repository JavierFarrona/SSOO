
# Makefile generado automáticamente para múltiples archivos

CXX = g++
CXXFLAGS = -std=c++14 -Wall -g
LDFLAGS =
TARGET = programa  # Cambia el nombre del ejecutable según lo necesites

# Todos los archivos .cc en el directorio actual
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

# Compilación completa del proyecto
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LDFLAGS)

# Regla para compilar cada archivo .cc a un .o
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpieza de archivos generados
clean:
	rm -f $(OBJ) $(TARGET)
	find . -name '*~' -exec rm {} \;

# Creación de un archivo comprimido de un directorio específico
tar:
	@if [ -z "$(DIR)" ]; then \
	    echo "Error: Debes especificar un directorio usando 'make tar DIR=<directorio>'"; \
	    exit 1; \
	fi
	tar -czvf "$(DIR).tgz" "$(DIR)"

# Eliminación del archivo comprimido
tar_clean:
	@if [ -z "$(DIR)" ]; then \
	    echo "Error: Debes especificar un directorio usando 'make tar_clean DIR=<directorio>'"; \
	    exit 1; \
	fi
	rm -f "$(DIR).tgz"

# Commit y push a git con un mensaje
git:
	@if [ -z "$(m)" ]; then \
	    echo "Error: Debes especificar un mensaje de commit usando 'make git m=\"<mensaje>\"'"; \
	    exit 1; \
	fi
	git add .
	git commit -m "$(m)"
	git push

