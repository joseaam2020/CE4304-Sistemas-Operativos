#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "init.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Uso: %s <shm_name> <modo: manual|auto> [periodo]\n",
            argv[0]);
    return 1;
  }

  const char *shm_name = argv[1];
  const char *modo = argv[2];
  int automatico = 0;
  int periodo = 1; // valor por defecto

  if (strcmp(modo, "manual") == 0) {
    automatico = 0;
  } else if (strcmp(modo, "auto") == 0) {
    automatico = 1;
    if (argc >= 4) {
      periodo = atoi(argv[3]);
      if (periodo <= 0) {
        fprintf(stderr, "Error: periodo debe ser un entero positivo.\n");
        return 1;
      }
    }
  } else {
    fprintf(stderr, "Error: modo inválido. Use 'manual' o 'auto'.\n");
    return 1;
  }

  // Abrir memoria compartida
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd == -1) {
    perror("Error: no se encontró memoria compartida con nombre ingresado");
    return 1;
  }

  // Leer buffer_size
  struct SharedTable *table =
      (struct SharedTable *)mmap(NULL, sizeof(struct SharedTable),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (table == MAP_FAILED) {
    perror("Error: no se pudo leer tabla de memoria compartida");
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
    perror("Error: no se pudo mapear memoria completa");
    close(fd);
    return 1;
  }

  table = (struct SharedTable *)addr;
  struct BufferPosition *buffer =
      (struct BufferPosition *)((char *)addr + table_size);

  // Loop principal
  while (1) {
    // Modo manual: esperar enter
    if (!automatico) {
      printf("\n[Modo Manual] Presiona Enter para leer un dato...");
      while (getchar() != '\n')
        ; // Esperar enter
    } else {
      sleep(periodo); // Esperar el periodo definido
    }

    // Verificar finalización
    sem_wait(&table->sem_finalizado);
    short fin = table->finalizado;
    sem_post(&table->sem_finalizado);
    if (fin)
      break;

    // Obtener índice del buffer
    sem_wait(&table->sem_receptor_index);
    int my_index = table->receptor_index;
    table->receptor_index = (table->receptor_index + 1) % buffer_size;
    sem_post(&table->sem_receptor_index);

    // Leer datos del buffer
    sem_wait(&buffer[my_index].sem_read);

    char read_c = buffer[my_index].letter;
    int read_index = buffer[my_index].index;
    time_t read_time_stamp = buffer[my_index].time_stamp;

    sem_post(&buffer[my_index].sem_write);

    // Mostrar resultados
    printf("Receptor leyó buffer[%d]: '%c'\n", read_index, read_c);
    printf("Hora de escritura: %s", asctime(localtime(&read_time_stamp)));
    printf("Comparando índices: receptor = %d, buffer = %d\n", my_index,
           read_index);
  }

  // Limpieza final
  munmap(addr, total_size);
  close(fd);

  printf("Receptor finalizado.\n");
  return 0;
}
