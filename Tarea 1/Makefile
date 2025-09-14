# Compilador
CC = gcc

# Directorios
SRC_DIR = src
BUILD_DIR = build

# Flags de compilación
CFLAGS = -Wall -Wextra -g

# Librerías para el servidor
LIBS_SERVER = -lpthread

# Archivos fuente
SERVER_SRCS = $(SRC_DIR)/ImagServer.c $(SRC_DIR)/Conf_lect.c
CLIENT_SRCS = $(SRC_DIR)/client.c

# Ejecutables
SERVER_BIN = $(BUILD_DIR)/server
CLIENT_BIN = $(BUILD_DIR)/client

# Regla por defecto
all: $(SERVER_BIN) $(CLIENT_BIN)

# Compilar servidor
$(SERVER_BIN): $(SERVER_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_SERVER)

# Compilar cliente
$(CLIENT_BIN): $(CLIENT_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Limpiar binarios
clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean
