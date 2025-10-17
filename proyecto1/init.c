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

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "init.h"

void usage(const char *prog) {
  fprintf(stderr, "Uso: %s <shm_name> <file_path> <buf_size>\n", prog);
  fprintf(stderr, "  shm_name: nombre POSIX (ej: /mi_shm)\n");
  fprintf(stderr, "  file_path: ruta a archivo a leer\n");
  fprintf(stderr, "  buf_size: tamaño del buffer (ej: 10)\n");
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  const char *shm_name = argv[1];
  const char *file_path = argv[2];

  int buffer_size;
  int result = sscanf(argv[3], "%d", &buffer_size);

  if (result != 1 || buffer_size <= 0) {
    fprintf(
        stderr,
        "Error: El argumento '%s' no es un número entero positivo válido.\n",
        argv[3]);
    return EXIT_FAILURE;
  }

  if (shm_name[0] != '/') {
    fprintf(stderr, "Warning: nombre shm debería comenzar con '/'. Continuando "
                    "de todas formas.\n");
  }

  // 1) Crear/abrir el objeto POSIX shared memory
  int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0666);
  if (shm_fd == -1) {
    if (errno == EEXIST) {
      fprintf(stderr, "El objeto de memoria compartida '%s' ya existe.\n",
              shm_name);
    } else {
      perror("shm_open");
    }
    return EXIT_FAILURE;
  }

  // Calcular tamaños
  size_t table_size = sizeof(struct SharedTable);
  size_t total_buffer_size = buffer_size * sizeof(struct BufferPosition);
  size_t file_path_size = (strlen(file_path) + 1) * sizeof(char);
  size_t total_size = table_size + total_buffer_size + file_path_size;

  // Establecer tamaño de memoria compartida
  if (ftruncate(shm_fd, total_size) == -1) {
    perror("ftruncate");
    close(shm_fd);
    shm_unlink(shm_name);
    return EXIT_FAILURE;
  }

  // Mapear en memoria
  void *addr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    close(shm_fd);
    shm_unlink(shm_name);
    return EXIT_FAILURE;
  }

  //  Inicializar estructuras
  memset(addr, 0, total_size);

  struct SharedTable *table = (struct SharedTable *)addr;
  struct BufferPosition *buffer =
      (struct BufferPosition *)((char *)addr + table_size);
  char *stored_path = (char *)addr + table_size + total_buffer_size;

  // Inicializar tabla
  table->read_pos = 0;
  table->end_program = 0;
  table->emiter_index = 0;
  table->receptor_index = 0;
  table->buffer_size = buffer_size;
  table->finalizado = 0;
  table->transfer_char = 0;
  table->num_emiters = 0;
  table->num_receptors = 0;
  table->num_emiters_closed = 0;
  table->num_receptors_closed = 0;
  table->num_emiters_dead = 0;
  table->num_receptors_dead = 0;

  // Inicializar semáforos de la tabla
  sem_init(&table->sem_read_pos, 1, 1);
  sem_init(&table->sem_end_program, 1, 1);
  sem_init(&table->sem_emiter_index, 1, 1);
  sem_init(&table->sem_receptor_index, 1, 1);
  sem_init(&table->sem_finalizado, 1, 1);
  sem_init(&table->sem_transfer_char, 1, 1);
  sem_init(&table->sem_num_emiters, 1, 1);
  sem_init(&table->sem_num_receptors, 1, 1);
  sem_init(&table->sem_num_emiters_closed, 1, 1);
  sem_init(&table->sem_num_receptors_closed, 1, 1);
  sem_init(&table->sem_wait_all_emiters, 1, 0);
  sem_init(&table->sem_wait_all_receptors, 1, 0);
  sem_init(&table->sem_num_emiters_dead, 1, 1);
  sem_init(&table->sem_num_receptors_dead, 1, 1);

  // Inicializar posiciones del buffer
  for (int i = 0; i < buffer_size; i++) {
    buffer[i].letter = '\0';
    buffer[i].index = i;
    buffer[i].time_stamp = 0;
    sem_init(&buffer[i].sem_read, 1, 0);
    sem_init(&buffer[i].sem_write, 1, 1);
  }

  // Copiar file_path
  strcpy(stored_path, file_path);
  printf(BLUE "[======================================================]" RESET
              "\n");
  printf(BRIGHT_CYAN "[Sistema]" YELLOW
                     "→ Memoria compartida creada exitosamente:\n");
  printf("  " BRIGHT_CYAN "• " YELLOW "[Nombre]→" BRIGHT_WHITE "%s\n",
         shm_name);
  printf("  " BRIGHT_CYAN "• " YELLOW "[Tamaño total]→" BRIGHT_WHITE
         "%zu bytes\n",
         total_size);
  printf("  " BRIGHT_CYAN "• " YELLOW "[Buffer_size]→" BRIGHT_WHITE
         "%d posiciones\n",
         buffer_size);
  printf("  " BRIGHT_CYAN "• " YELLOW "[Archivo de metadatos]→" BRIGHT_WHITE
         "%s\n",
         file_path);

  printf(BLUE
         "[======================================================]\n" RESET);

  // Sincronizar y liberar
  msync(addr, total_size, MS_SYNC);
  munmap(addr, total_size);
  close(shm_fd);

  return EXIT_SUCCESS;
}

// cerrar la memoria: ls -la /dev/shm/    ver cuales estan activas
// rm /dev/shm/"nombre de la memoria"
