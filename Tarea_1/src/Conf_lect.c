#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH 256
#define MAX_LOG_LEVEL 32

// ---- Estructura de configuración ----
struct conf {
    int port;
    char log_level[MAX_LOG_LEVEL];
    char histograma[MAX_PATH];
    char colores[MAX_PATH];
    char log_file[MAX_PATH];
};

// ---- Función para leer el archivo de configuración ----
struct conf read_config(const char* filename) {
    struct conf cfg;

    // Valores por defecto
    cfg.port = 1717;
    strcpy(cfg.log_level, "INFO");
    strcpy(cfg.histograma, "");
    strcpy(cfg.colores, "");
    strcpy(cfg.log_file, "");

    FILE* file = fopen(filename, "r");
    if (!file) {
        // No se pudo abrir archivo, devolvemos defaults
        return cfg;
    }

    char line[512];

    while (fgets(line, sizeof(line), file)) {
        // Ignorar comentarios
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        // Saltar espacios iniciales
        char* ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        // ---- PORT ----
        if (strncmp(ptr, "PORT=", 5) == 0) {
            ptr += 5;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            int tmp_port = atoi(ptr);
            if (tmp_port > 0 && tmp_port < 65536) {
                cfg.port = tmp_port;
            }
        }
        // ---- LOG_LEVEL ----
        else if (strncmp(ptr, "LOG_LEVEL=", 10) == 0) {
            ptr += 10;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            strncpy(cfg.log_level, ptr, MAX_LOG_LEVEL - 1);
            cfg.log_level[strcspn(cfg.log_level, "\r\n")] = 0;
        }
        // ---- HISTOGRAMA ----
        else if (strncmp(ptr, "HISTOGRAMA=", 11) == 0) {
            ptr += 11;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            strncpy(cfg.histograma, ptr, MAX_PATH - 1);
            cfg.histograma[strcspn(cfg.histograma, "\r\n")] = 0;
        }
        // ---- COLORES ----
        else if (strncmp(ptr, "COLORES=", 8) == 0) {
            ptr += 8;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            strncpy(cfg.colores, ptr, MAX_PATH - 1);
            cfg.colores[strcspn(cfg.colores, "\r\n")] = 0;
        }
        // ---- LOG_FILE ----
        else if (strncmp(ptr, "LOG_FILE=", 9) == 0) {
            ptr += 9;
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            strncpy(cfg.log_file, ptr, MAX_PATH - 1);
            cfg.log_file[strcspn(cfg.log_file, "\r\n")] = 0;
        }
    }

    fclose(file);
    return cfg;
}

