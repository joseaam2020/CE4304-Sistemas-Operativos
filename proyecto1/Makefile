# Nombre de los ejecutables
BIN_DIR = build
BINS = $(BIN_DIR)/init $(BIN_DIR)/emisor $(BIN_DIR)/receptor

# Archivos fuente
SRCS = init.c emisor.c receptor.c

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 

# Regla principal
all: $(BIN_DIR) $(BINS)

# Crear carpeta build si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Regla para compilar cada ejecutable
$(BIN_DIR)/%: %.c init.h
	$(CC) $(CFLAGS) -o $@ $<

# Limpiar los binarios
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
