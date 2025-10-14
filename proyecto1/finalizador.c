#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#define GRAY    "\033[1;90m"

#define BRIGHT_RED      "\033[1;31m"
#define BRIGHT_GREEN    "\033[1;32m"
#define BRIGHT_YELLOW   "\033[1;33m"
#define BRIGHT_BLUE     "\033[1;34m"
#define BRIGHT_MAGENTA  "\033[1;35m"
#define BRIGHT_CYAN     "\033[1;36m"
#define BRIGHT_WHITE    "\033[1;37m"

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

  printf(BLUE "[======================================================]" RESET "\n");
  printf("  " BRIGHT_CYAN "• " YELLOW "[Nombre]→ " BRIGHT_WHITE "%s\n", shm_name);

  // Abrir memoria compartida existente
  int fd = shm_open(shm_name, O_RDWR, 0);
  if (fd == -1) {
    perror(RED "Error: no se encontró la memoria compartida" RESET);
    return 1;
  }

  // Mapear la tabla
  struct SharedTable *table = (struct SharedTable *)mmap(
      NULL, sizeof(struct SharedTable),
      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (table == MAP_FAILED) {
    perror(RED "Error: no se pudo mapear la tabla de memoria compartida" RESET);
    close(fd);
    return 1;
  }

  // Activo la variable
  sem_wait(&table->sem_finalizado);
  table->finalizado = 1;
  sem_post(&table->sem_finalizado);

  printf(GREEN "Finalizador ejecutado\n" RESET);
  printf(BLUE "[======================================================]" RESET "\n");

  // Sincronizar y liberar
  msync(table, sizeof(struct SharedTable), MS_SYNC);
  munmap(table, sizeof(struct SharedTable));
  close(fd);

  if (shm_unlink(shm_name) == 0) {
    printf(BRIGHT_YELLOW "Memoria compartida eliminada correctamente del sistema.\n" RESET);
  } else {
    perror(RED "Error al eliminar la memoria compartida" RESET);
  }


  return 0;
}
