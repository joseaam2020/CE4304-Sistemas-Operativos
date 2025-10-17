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
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <shm_name>\n", argv[0]);
    return 1;
  }

  const char *shm_name = argv[1];

  printf(BLUE "[======================================================]" RESET
              "\n");
  printf("  " BRIGHT_CYAN "• " YELLOW "[Nombre]→ " BRIGHT_WHITE "%s\n",
         shm_name);

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

  // Activo la variable
  sem_wait(&table->sem_finalizado);
  table->finalizado = 1;
  sem_post(&table->sem_finalizado);

  // Se leen numero de receptores totales
  int total_receptors;
  sem_wait(&table->sem_num_receptors);
  total_receptors = table->num_receptors;
  sem_post(&table->sem_num_receptors);

  // Se leen numero de emisores totales
  int total_emiters;
  sem_wait(&table->sem_num_emiters);
  total_emiters = table->num_emiters;
  sem_post(&table->sem_num_emiters);

  // Se leen numero de receptores muertos
  int total_receptors_dead;
  sem_wait(&table->sem_num_receptors_dead);
  total_receptors_dead = table->num_receptors_dead;
  sem_post(&table->sem_num_receptors_dead);

  // Se leen numero de emisores muertos
  int total_emiters_dead;
  sem_wait(&table->sem_num_emiters_dead);
  total_emiters_dead = table->num_emiters_dead;
  sem_post(&table->sem_num_emiters_dead);

  // Se abren los semaforos de todos para que entren y verifiquen finalizacion
  for (short i = 0; i < buffer_size; i++) {
    for (short j = 0; j < (total_emiters - total_emiters_dead); j++) {
      sem_post(&buffer[i].sem_write);
    }

    for (short j = 0; j < (total_receptors - total_receptors_dead); j++) {
      sem_post(&buffer[i].sem_read);
    }
  }

  // Esperar a que cierren todos los procesos
  if (total_emiters - total_emiters_dead > 0) {
    printf(GREEN "Esperando finalizacion emisores...\n");
    sem_wait(&table->sem_wait_all_emiters);
  }

  if (total_receptors - total_receptors_dead > 0) {
    printf(GREEN "Esperando finalizacion receptores...\n");
    sem_wait(&table->sem_wait_all_receptors);
  }

  // Se leen caracteres transferidos
  int transfered_char;
  sem_wait(&table->sem_transfer_char);
  transfered_char = table->transfer_char;
  sem_post(&table->sem_transfer_char);

  // Se leen numero de receptores totales
  int live_receptors;
  sem_wait(&table->sem_num_receptors_closed);
  live_receptors = table->num_receptors_closed;
  sem_post(&table->sem_num_receptors_closed);

  // Se leen numero de emisores livees
  int live_emiters;
  sem_wait(&table->sem_num_emiters_closed);
  live_emiters = table->num_emiters_closed;
  sem_post(&table->sem_num_emiters_closed);

  // Leer caracteres aun en memoria compartida
  int num_shm_characters = 0;

  for (short i = 0; i < table->buffer_size; i++) {
    if (sem_trywait(&buffer[i].sem_read) == 0) {
      // Hay algo para leer
      if (buffer[i].letter != 0) {
        num_shm_characters++;
      }

      sem_post(&buffer[i].sem_read);
    }
  }

  printf(YELLOW "Caracteres transferidos: " BLUE "%d\n", transfered_char);
  printf(YELLOW "Receptores totales:      " BLUE "%d\n", total_receptors);
  printf(YELLOW "Emisores totales:        " BLUE "%d\n", total_emiters);
  printf(YELLOW "Receptores activos:      " BLUE "%d\n", live_receptors);
  printf(YELLOW "Emisores activos:        " BLUE "%d\n", live_emiters);
  printf(YELLOW "Caracteres en memoria:   " BLUE "%d\n", num_shm_characters);

  printf(GREEN "Finalizador ejecutado\n" RESET);
  printf(BLUE "[======================================================]" RESET
              "\n");

  // Sincronizar y liberar
  msync(table, sizeof(struct SharedTable), MS_SYNC);
  munmap(table, sizeof(struct SharedTable));
  close(fd);

  if (shm_unlink(shm_name) == 0) {
    printf(BRIGHT_YELLOW
           "Memoria compartida eliminada correctamente del sistema.\n" RESET);
  } else {
    perror(RED "Error al eliminar la memoria compartida" RESET);
  }

  return 0;
}
