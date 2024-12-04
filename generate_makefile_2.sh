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
MAKEFILE_CONTENT="# Makefile generado automáticamente con soporte para archivos objeto en /trash

CXX = g++
CXXFLAGS = -std=c++14 -Wall -g -Ilib
LDFLAGS =
TARGET = programa # Cambia el nombre del ejecutable según lo necesites

# Carpetas
SRC_DIR = src
LIB_DIR = lib
OBJ_DIR = trash

# Crear carpeta para los archivos objeto si no existe
\$(shell mkdir -p \$(OBJ_DIR))

# Archivos fuente y objetos
SRC = \$(wildcard \$(SRC_DIR)/*.cpp)
LIB = \$(wildcard \$(LIB_DIR)/*.cpp)
OBJ = \$(patsubst \$(SRC_DIR)/%.cpp,\$(OBJ_DIR)/%.o,\$(SRC)) \\
      \$(patsubst \$(LIB_DIR)/%.cpp,\$(OBJ_DIR)/%.o,\$(LIB))

# Compilación del proyecto
all: \$(TARGET)

\$(TARGET): \$(OBJ)
	\$(CXX) -o \$@ \$^ \$(LDFLAGS)

# Reglas para compilar cada archivo fuente a objeto
\$(OBJ_DIR)/%.o: \$(SRC_DIR)/%.cpp
	\$(CXX) \$(CXXFLAGS) -c \$< -o \$@

\$(OBJ_DIR)/%.o: \$(LIB_DIR)/%.cpp
	\$(CXX) \$(CXXFLAGS) -c \$< -o \$@

# Limpieza de archivos generados
clean:
	rm -rf \$(OBJ_DIR) \$(TARGET)

# Formateo de archivos según la guía de estilo de Google
format:
	@files=\$$(find \$(SRC_DIR) \$(LIB_DIR) -type f \\( -name '*.cpp' -o -name '*.h' \\)); \\
	if [ -n \"\$\$files\" ]; then \\
	    echo \"Formateando los siguientes archivos:\"; \\
	    echo \"\$\$files\"; \\
	    clang-format -i --style=Google \$\$files; \\
	else \\
	    echo \"No hay archivos para formatear en \$(SRC_DIR) o \$(LIB_DIR).\"; \\
	fi

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
"

# Guarda el Makefile en la ruta destino
echo "$MAKEFILE_CONTENT" > "$RUTA_DESTINO/Makefile"

echo "Makefile generado en $RUTA_DESTINO/Makefile"
