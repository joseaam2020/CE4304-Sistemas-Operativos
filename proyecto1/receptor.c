#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "init.h"

// Inicializacion
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <shm_name>\n", argv[0]);
    return 1;
  }

  const char *shm_name = argv[1];

  // Abrir memoria compartida
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd == -1) {
    perror("Error: no se encontro memoria compartida con nombre ingresado");
    return 1;
  }

  // Se lee la tabla para abrir el buffer a leer
  struct SharedTable *table =
      (struct SharedTable *)mmap(NULL, sizeof(struct SharedTable),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (table == MAP_FAILED) {
    perror("Error: no se pudo leer tabla de memoria compartida");
    close(fd);
    return 1;
  }

  // Se consigue tamanio del buffer para mapear la memoria compartida completa
  int buffer_size = table->buffer_size;
  munmap(table, sizeof(struct SharedTable));

  // Mapear toda la memoria compartida completa
  size_t table_size = sizeof(struct SharedTable);
  size_t buffer_bytes = buffer_size * sizeof(struct BufferPosition);
  size_t total_size = table_size + buffer_bytes + 256;

  void *addr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    perror("Error: no se pudo mapear tabla de memoria completa");
    close(fd);
    return 1;
  }

  table = (struct SharedTable *)addr;
  struct BufferPosition *buffer =
      (struct BufferPosition *)((char *)addr + table_size);

  // char *file_path = (char *)addr + table_size + buffer_bytes;
  // Posible modificacion, es necesario guardar el tamanio del path
  // de la memoria para hacer mejor uso de la memoria

  // printf("[INFO] Archivo: %s\n", file_path);

  //  Obtener índice para leer en el buffer y actualizar de manera circular
  sem_wait(&table->sem_receptor_index);
  int my_index = table->receptor_index;
  table->receptor_index = (table->receptor_index + 1) % buffer_size;
  sem_post(&table->sem_receptor_index);

  printf("Receptor iniciado, leyendo posicion %d del buffer.\n", my_index);

  // Lectura de elemenetos en buffer
  char read_c;
  int read_index;
  time_t read_time_stamp;

  // Leer carácter en su posición del buffer
  sem_wait(&buffer[my_index].sem_read);

  read_c = buffer[my_index].letter;
  read_index = buffer[my_index].index;
  read_time_stamp = buffer[my_index].time_stamp;

  sem_post(&buffer[my_index].sem_write);

  printf("Receptor en buffer[%d] leyo '%c'.\n", read_index, read_c);
  printf("Hora de escritura: %s", asctime(localtime(&read_time_stamp)));
  printf("Compartando indices %d == %d\n", my_index, read_index);

  // Limpieza
  munmap(addr, total_size);
  close(fd);

  return 0;
}
