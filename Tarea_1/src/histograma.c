#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH 1024
#define MAX_VALUE 256

// --- Estructura de configuración ---
struct conf {
    int port;
    char log_level[32];
    char histograma[MAX_PATH];
    char colores[MAX_PATH];
    char log_file[MAX_PATH];
    char rojo[MAX_PATH];
    char verde[MAX_PATH];
    char azul[MAX_PATH];
};

// --- Parser de .conf ---
struct conf read_config(const char* filename) {
    struct conf cfg;

    // valores por defecto
    cfg.port = 1717;
    strcpy(cfg.log_level, "INFO");
    strcpy(cfg.histograma, "");
    strcpy(cfg.colores, "");
    strcpy(cfg.log_file, "");
    strcpy(cfg.rojo, "");
    strcpy(cfg.verde, "");
    strcpy(cfg.azul, "");

    FILE* file = fopen(filename, "r");
    if (!file) return cfg;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';
        char* ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        if (strncmp(ptr, "PORT=", 5) == 0) {
            ptr += 5; cfg.port = atoi(ptr);
        } else if (strncmp(ptr, "LOG_LEVEL=", 10) == 0) {
            ptr += 10; strncpy(cfg.log_level, ptr, 31); cfg.log_level[strcspn(cfg.log_level, "\r\n")] = 0;
        } else if (strncmp(ptr, "HISTOGRAMA=", 11) == 0) {
            ptr += 11; strncpy(cfg.histograma, ptr, MAX_PATH-1); cfg.histograma[strcspn(cfg.histograma, "\r\n")] = 0;
        } else if (strncmp(ptr, "COLORES=", 8) == 0) {
            ptr += 8; strncpy(cfg.colores, ptr, MAX_PATH-1); cfg.colores[strcspn(cfg.colores, "\r\n")] = 0;
        } else if (strncmp(ptr, "ROJO=", 5) == 0) {
            ptr += 5; strncpy(cfg.rojo, ptr, MAX_PATH-1); cfg.rojo[strcspn(cfg.rojo, "\r\n")] = 0;
        } else if (strncmp(ptr, "VERDE=", 6) == 0) {
            ptr += 6; strncpy(cfg.verde, ptr, MAX_PATH-1); cfg.verde[strcspn(cfg.verde, "\r\n")] = 0;
        } else if (strncmp(ptr, "AZUL=", 5) == 0) {
            ptr += 5; strncpy(cfg.azul, ptr, MAX_PATH-1); cfg.azul[strcspn(cfg.azul, "\r\n")] = 0;
        } else if (strncmp(ptr, "LOG_FILE=", 9) == 0) {
            ptr += 9; strncpy(cfg.log_file, ptr, MAX_PATH-1); cfg.log_file[strcspn(cfg.log_file, "\r\n")] = 0;
        }
    }
        

    fclose(file);
    printf("Rutas leídas:\nROJO='%s'\nVERDE='%s'\nAZUL='%s'\n", cfg.rojo, cfg.verde, cfg.azul);
    return cfg;
}

// --- Función principal ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nombre_imagen>\n", argv[0]);
        return 1;
    }

    char *archivo = argv[1];
    struct conf cfg = read_config("imageserver.conf");

    int width, height, channels;
    unsigned char *img = stbi_load(archivo, &width, &height, &channels, 3);
    if (!img) {
        printf("Error al cargar la imagen %s\n", archivo);
        return 1;
    }

    int histR[MAX_VALUE] = {0}, histG[MAX_VALUE] = {0}, histB[MAX_VALUE] = {0};

    for (int i = 0; i < width * height * 3; i += 3) {
        unsigned char r = img[i + 0];
        unsigned char g = img[i + 1];
        unsigned char b = img[i + 2];

        histR[r]++; histG[g]++; histB[b]++;
    }

    stbi_image_free(img);

    long sumR = 0, sumG = 0, sumB = 0;
    for (int i = 0; i < MAX_VALUE; i++) {
        sumR += histR[i] * i;
        sumG += histG[i] * i;
        sumB += histB[i] * i;
    }

    printf("Total de píxeles ponderados:\nR: %ld\nG: %ld\nB: %ld\n", sumR, sumG, sumB);

    char destino[MAX_PATH];
    char color[16];

    if (sumR >= sumG && sumR >= sumB) {
        strcpy(color, "rojo");
        snprintf(destino, sizeof(destino), "%s/%s", cfg.rojo, archivo);
    } else if (sumG >= sumR && sumG >= sumB) {
        strcpy(color, "verde");
        snprintf(destino, sizeof(destino), "%s/%s", cfg.verde, archivo);
    } else {
        strcpy(color, "azul");
        snprintf(destino, sizeof(destino), "%s/%s", cfg.azul, archivo);
    }

    printf("Color predominante: %s\n", color);

    if (rename(archivo, destino) == 0) {
        printf("Imagen movida a %s\n", destino);
    } else {
        perror("Error moviendo la imagen");
    }

    return 0;
}
