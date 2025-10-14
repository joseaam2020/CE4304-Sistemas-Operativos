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

  // 2) Calcular tamaños
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

  // 3) Mapear en memoria
  void *addr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    close(shm_fd);
    shm_unlink(shm_name);
    return EXIT_FAILURE;
  }

  // 4) Inicializar estructuras
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

  // Inicializar semáforos de la tabla
  sem_init(&table->sem_read_pos, 1, 1);
  sem_init(&table->sem_end_program, 1, 1);
  sem_init(&table->sem_emiter_index, 1, 1);
  sem_init(&table->sem_receptor_index, 1, 1);
  sem_init(&table->sem_finalizado, 1, 1);

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

  // 5) Crear archivo de metadatos (opcional)
  int meta_fd = open(file_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (meta_fd == -1) {
    perror("open(file_path)");
  } else {
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
                     "shm_name=%s\nsize=%zu\nbuffer_size=%d\npath=%s\n",
                     shm_name, total_size, buffer_size, file_path);
    if (n > 0) {
      write(meta_fd, buf, (size_t)n);
    }
    close(meta_fd);
  }

  printf("Memoria compartida creada exitosamente:\n");
  printf("  nombre: %s\n", shm_name);
  printf("  tamaño total: %zu bytes\n", total_size);
  printf("  buffer_size: %d posiciones\n", buffer_size);
  printf("  archivo de metadatos: %s\n", file_path);

  // Sincronizar y liberar
  msync(addr, total_size, MS_SYNC);
  munmap(addr, total_size);
  close(shm_fd);

  return EXIT_SUCCESS;
}

// cerrar la memoria: ls -la /dev/shm/    ver cuales estan activas
// rm /dev/shm/"nombre de la memoria"
