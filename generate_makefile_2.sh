#!/bin/bash

# Verifica que se haya proporcionado un argumento (la ruta de destino)
if [ "$#" -ne 1 ]; then
    echo "Uso: $0 <ruta_destino>"
    exit 1
fi

# Ruta de destino
RUTA_DESTINO="$1"

# Crea el directorio si no existe
mkdir -p "$RUTA_DESTINO"

# Contenido del Makefile actualizado
MAKEFILE_CONTENT="
# Makefile generado automáticamente para múltiples archivos

CXX = g++
CXXFLAGS = -std=c++14 -Wall -g
LDFLAGS =
TARGET = programa  # Cambia el nombre del ejecutable según lo necesites

# Todos los archivos .cc en el directorio actual
SRC = \$(wildcard *.cpp)
OBJ = \$(SRC:.cpp=.o)

# Compilación completa del proyecto
all: \$(TARGET)

\$(TARGET): \$(OBJ)
	\$(CXX) -o \$(TARGET) \$(OBJ) \$(LDFLAGS)

# Regla para compilar cada archivo .cc a un .o
%.o: %.cc
	\$(CXX) \$(CXXFLAGS) -c \$< -o \$@

# Limpieza de archivos generados
clean:
	rm -f \$(OBJ) \$(TARGET)
	find . -name '*~' -exec rm {} \\;

# Creación de un archivo comprimido de un directorio específico
tar:
	@if [ -z \"\$(DIR)\" ]; then \\
	    echo \"Error: Debes especificar un directorio usando 'make tar DIR=<directorio>'\"; \\
	    exit 1; \\
	fi
	tar --exclude=\"\$(DIR).tgz\" -czvf \"\$(DIR).tgz\" \"\$(DIR)\"

# Eliminación del archivo comprimido
tar_clean:
	@if [ -z \"\$(DIR)\" ]; then \\
	    echo \"Error: Debes especificar un directorio usando 'make tar_clean DIR=<directorio>'\"; \\
	    exit 1; \\
	fi
	rm -f \"\$(DIR).tgz\"

# Commit y push a git con un mensaje
git:
	@if [ -z \"\$(m)\" ]; then \\
	    echo \"Error: Debes especificar un mensaje de commit usando 'make git m=\\\"<mensaje>\\\"'\"; \\
	    exit 1; \\
	fi
	git add .
	git commit -m \"\$(m)\"
	git push

# Regla para formatear todos los archivos según la guía de estilo de Google
format:
	@echo \"Formateando archivos según la guía de estilo de Google...\"
	clang-format -i --style=Google *.cpp *.h
"

# Guarda el Makefile en la ruta destino
echo "$MAKEFILE_CONTENT" > "$RUTA_DESTINO/Makefile"

echo "Makefile generado en $RUTA_DESTINO/Makefile"
