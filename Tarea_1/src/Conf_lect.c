#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int read_port_from_config(const char* filename) {
    int port = 1717; // Valor default

    FILE* file = fopen(filename, "r");
    if (!file) {
        // No se pudo abrir archivo, devolver por defecto
        return port;
    }

    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Ignorar comentarios
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        // Saltar espacios iniciales
        char* ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        // Buscar "PORT="
        if (strncmp(ptr, "PORT=", 5) == 0) {
            ptr += 5;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            int tmp_port = atoi(ptr);
            if (tmp_port > 0 && tmp_port < 65536) {
                port = tmp_port; // Sobrescribir solo si es válido
            }
            break;
        }
    }

    fclose(file);
    return port;
}
