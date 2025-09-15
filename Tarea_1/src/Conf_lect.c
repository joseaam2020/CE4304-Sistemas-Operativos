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
    char rojo[MAX_PATH];
    char verde[MAX_PATH];
    char azul[MAX_PATH];


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
    strcpy(cfg.rojo, "");
    strcpy(cfg.verde, "");
    strcpy(cfg.azul, "");

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("No se pudo abrir el archivo de configuración: %s\n", filename);
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

        // Leer cada campo
        if (strncmp(ptr, "PORT=", 5) == 0) {
            cfg.port = atoi(ptr + 5);
        } else if (strncmp(ptr, "LOG_LEVEL=", 10) == 0) {
            strncpy(cfg.log_level, ptr + 10, 31);
            cfg.log_level[strcspn(cfg.log_level, "\r\n")] = 0;
        } else if (strncmp(ptr, "HISTOGRAMA=", 11) == 0) {
            strncpy(cfg.histograma, ptr + 11, MAX_PATH - 1);
            cfg.histograma[strcspn(cfg.histograma, "\r\n")] = 0;
        } else if (strncmp(ptr, "COLORES=", 8) == 0) {
            strncpy(cfg.colores, ptr + 8, MAX_PATH - 1);
            cfg.colores[strcspn(cfg.colores, "\r\n")] = 0;
        } else if (strncmp(ptr, "ROJO=", 5) == 0) {
            strncpy(cfg.rojo, ptr + 5, MAX_PATH - 1);
            cfg.rojo[strcspn(cfg.rojo, "\r\n")] = 0;
        } else if (strncmp(ptr, "VERDE=", 6) == 0) {
            strncpy(cfg.verde, ptr + 6, MAX_PATH - 1);
            cfg.verde[strcspn(cfg.verde, "\r\n")] = 0;
        } else if (strncmp(ptr, "AZUL=", 5) == 0) {
            strncpy(cfg.azul, ptr + 5, MAX_PATH - 1);
            cfg.azul[strcspn(cfg.azul, "\r\n")] = 0;
        } else if (strncmp(ptr, "LOG_FILE=", 9) == 0) {
            strncpy(cfg.log_file, ptr + 9, MAX_PATH - 1);
            cfg.log_file[strcspn(cfg.log_file, "\r\n")] = 0;

        }
    }

    fclose(file);

    // Imprimir las rutas correctamente cargadas
    printf("Rutas leídas:\nROJO='%s'\nVERDE='%s'\nAZUL='%s'\n", cfg.rojo, cfg.verde, cfg.azul);

    return cfg;
}
