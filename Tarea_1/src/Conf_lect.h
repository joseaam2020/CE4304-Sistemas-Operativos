#ifndef CONF_LECT_H
#define CONF_LECT_H

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

// ---- Prototipo de función ----
struct conf read_config(const char* filename);

#endif /* CONF_LECT_H */
