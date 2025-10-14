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
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <shm_name> <key_encriptacion> <modo: manual|auto> [periodo ms]\n",
            argv[0]);
    return 1;
  }

  const char *shm_name = argv[1];
  const char *modo = argv[3];
  int automatico = 0;
  int periodo = 1000; // valor por defecto

  int key;
  int result = sscanf(argv[2], "%d",&key);
      if (result != 1 || key > 256) {
            fprintf(stderr, "Error: El argumento '%s' no es un número de 8bits.\n", argv[2]);
             return EXIT_FAILURE; }



  // Leer modo
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
      printf(BRIGHT_CYAN "[Modo Manual]" RESET " → " YELLOW "Presiona Enter para leer un dato..." RESET "\n");
      while (getchar() != '\n')
        ; // Esperar enter
    } else {
      usleep(periodo * 1000); // Esperar el periodo definido
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
    printf(CYAN "[Receptor]" RESET " → ENCRIPTADo: " YELLOW "'%c'" RESET "\n", read_c);

    read_c^= key;
    int transferchar = table-> transfer_char; 
    int read_index = buffer[my_index].index;
    time_t read_time_stamp = buffer[my_index].time_stamp;
    sem_post(&buffer[my_index].sem_write);

    sem_wait(&table->sem_transfer_char);
    table->transfer_char++;
    sem_post(&table->sem_transfer_char);


    // Mostrar resultados
    printf(BLUE"[======================================================]" RESET "\n");
    printf(CYAN "[Receptor]" RESET " → Leyó " YELLOW "buffer[%d]" RESET ": '%c'\n", read_index, read_c);
    printf(GREEN "[Hora de lectura]" RESET " → %s", asctime(localtime(&read_time_stamp)));
    printf(BRIGHT_MAGENTA "[Comparando índices]" RESET " → receptor = " CYAN "%d" YELLOW ", buffer = " YELLOW "%d" RESET "\n",
       my_index, read_index);
    printf(BRIGHT_RED"[Hora de escritura]→%s", asctime(localtime(&read_time_stamp)));
    printf(BRIGHT_GREEN "[Total de caracteres transferidos]→ " BRIGHT_WHITE "%d\n", transferchar);

    printf(BLUE "[======================================================]" RESET "\n");
  }

  // Limpieza final
  munmap(addr, total_size);
  close(fd);

  printf(BRIGHT_WHITE "\n[Receptor finalizado correctamente]\n" RESET);
  return 0;
}
