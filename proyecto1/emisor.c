
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "init.h"

// Colores para prints
#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define GRAY "\033[1;90m"

#define BRIGHT_RED "\033[1;31m"
#define BRIGHT_GREEN "\033[1;32m"
#define BRIGHT_YELLOW "\033[1;33m"
#define BRIGHT_BLUE "\033[1;34m"
#define BRIGHT_MAGENTA "\033[1;35m"
#define BRIGHT_CYAN "\033[1;36m"
#define BRIGHT_WHITE "\033[1;37m"

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr,
            "Uso: %s <shm_name> <key_encriptacion> <modo: manual|auto> "
            "[periodo_ms]\n",
            argv[0]);
    return 1;
  }

  // Leer clave de encriptación
  int key;
  int result = sscanf(argv[2], "%d", &key);
  if (result != 1 || key < 0 || key > 255) {
    fprintf(stderr,
            "Error: El argumento '%s' no es un número de 8 bits válido.\n",
            argv[2]);
    return EXIT_FAILURE;
  }

  // Leer modo
  const char *modo = argv[3];
  int automatico = 0;
  int periodo = 1000; // milisegundos por defecto

  if (strcmp(modo, "manual") == 0) {
    automatico = 0;
  } else if (strcmp(modo, "auto") == 0) {
    automatico = 1;
    if (argc >= 5) {
      periodo = atoi(argv[4]);
      if (periodo <= 0) {
        fprintf(stderr, "Error: periodo debe ser un entero positivo.\n");
        return 1;
      }
    }
  } else {
    fprintf(stderr, "Error: modo inválido. Use 'manual' o 'auto'.\n");
    return 1;
  }

  const char *shm_name = argv[1];

  // Abrir memoria compartida
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd == -1) {
    perror("shm_open");
    return 1;
  }

  // Leer tabla para obtener buffer_size
  struct SharedTable *table =
      (struct SharedTable *)mmap(NULL, sizeof(struct SharedTable),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (table == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  int buffer_size = table->buffer_size;
  munmap(table, sizeof(struct SharedTable));

  // Mapear memoria completa
  size_t table_size = sizeof(struct SharedTable);
  size_t buffer_bytes = buffer_size * sizeof(struct BufferPosition);
  size_t total_size = table_size + buffer_bytes + 256;

  void *addr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap total");
    close(fd);
    return 1;
  }

  table = (struct SharedTable *)addr;
  struct BufferPosition *buffer =
      (struct BufferPosition *)((char *)addr + table_size);
  char *file_path = (char *)addr + table_size + buffer_bytes;

  printf(BLUE "[======================================================]" RESET
              "\n");
  printf(BRIGHT_CYAN "[INFO]" RESET " → Archivo: %s\n", file_path);

  // Abrir archivo
  int file_fd = open(file_path, O_RDONLY);
  if (file_fd == -1) {
    perror("open archivo");
    munmap(addr, total_size);
    close(fd);
    return 1;
  }

  // Loop principal
  while (1) {
    if (!automatico) {
      printf(GRAY "\n[Modo Manual] Presiona Enter para encriptar y "
                  "escribir...\n" RESET);
      while (getchar() != '\n')
        ;
    } else {
      usleep(periodo * 1000);
    }

    // Verificar finalización
    sem_wait(&table->sem_finalizado);
    short fin = table->finalizado;
    sem_post(&table->sem_finalizado);
    if (fin)
      break;

    // Obtener índice del emisor (circular)
    sem_wait(&table->sem_emiter_index);
    int my_index = table->emiter_index;
    table->emiter_index = (table->emiter_index + 1) % buffer_size;
    sem_post(&table->sem_emiter_index);

    printf(BRIGHT_GREEN "[Emisor %d]" RESET " → Iniciado.\n", my_index);

    // Obtener posición de lectura
    sem_wait(&table->sem_read_pos);
    unsigned long my_file_pos = table->read_pos++;
    sem_post(&table->sem_read_pos);

    // Leer caracter desde archivo
    char c;
    ssize_t nread = pread(file_fd, &c, 1, my_file_pos);
    if (nread != 1) {
      printf(RED "[Emisor %d] Fin del archivo o error de lectura.\n" RESET,
             my_index);
      break;
    }

    printf(BRIGHT_WHITE "[Char original]" RESET " → '%c'\n", c);

    // Encriptar
    c ^= key;
    printf(BRIGHT_MAGENTA "[Encriptado]" RESET " → '%c'\n", c);

    time_t write_time = time(NULL);

    // Escribir en el buffer
    sem_wait(&buffer[my_index].sem_write);
    buffer[my_index].letter = c;
    buffer[my_index].index = my_index;
    buffer[my_index].time_stamp = write_time;
    sem_post(&buffer[my_index].sem_read);

    printf(BRIGHT_CYAN "[Emisor %d]" RESET " → Escribió '%c' en buffer[%d]\n",
           my_index, c, my_index);
    printf(BRIGHT_RED "[Hora de escritura] → %s" RESET,
           asctime(localtime(&write_time)));
    printf(BLUE
           "[======================================================]\n" RESET);
  }

  // Limpieza
  close(file_fd);
  munmap(addr, total_size);
  close(fd);

  printf(GREEN "\n[Emisor finalizado correctamente]\n" RESET);
  return 0;
}
