//Colores para prints
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


#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "init.h"

// Inicializacion
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <shm_name> <key_encriptacion>\n", argv[0]);
        return 1;
    }
    int key;
    int result = sscanf(argv[2], "%d",&key);
   

        if (result != 1 || key > 256) {
            fprintf(stderr, "Error: El argumento '%s' no es un número de 8bits.\n", argv[2]);
             return EXIT_FAILURE; }

    const char *shm_name = argv[1];

    // Abrir memoria compartida
    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Se lee la tabla para abrir el archivo a leer
    struct SharedTable *table = (struct SharedTable *)mmap(NULL, sizeof(struct SharedTable),
                                                      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (table == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    int buffer_size = table->buffer_size;
    munmap(table, sizeof(struct SharedTable));

    // Mapear toda la memoria compartida completa
    size_t table_size = sizeof(struct SharedTable);
    size_t buffer_bytes = buffer_size * sizeof(struct BufferPosition);
    size_t total_size = table_size + buffer_bytes + 256;

    void *addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap total");
        close(fd);
        return 1;
    }

    table = (struct SharedTable *)addr;
    struct BufferPosition *buffer = (struct BufferPosition *)((char *)addr + table_size);
    char *file_path = (char *)addr + table_size + buffer_bytes;

    printf(BLUE "[======================================================]" RESET "\n");
    printf(BRIGHT_CYAN "[INFO] ]" RESET " → Archivo: %s\n" RESET, file_path);
    

    

    //  Obtener índice para escribir en el buffer

    sem_wait(&table->sem_emiter_index);
    int my_index = table->emiter_index;
    table->emiter_index = (table->emiter_index + 1) % buffer_size;
    sem_post(&table->sem_emiter_index);

    printf(BRIGHT_GREEN "[Emisor %d]" RESET " → Iniciado.\n"RESET, my_index);
   





    //  Reservar posición de lectura en el archivo
    sem_wait(&table->sem_read_pos);
    unsigned long my_file_pos = table->read_pos++;
    sem_post(&table->sem_read_pos);

    //  Leer un carácter del archivo
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        perror("open archivo");
        return 1;
    }

    //Lectura del doc y se imprime en el buffer 
    char c;
    ssize_t nread = pread(file_fd, &c, 1, my_file_pos);

       printf( BRIGHT_WHITE "[Char original]" RESET " → Resultado: " YELLOW "'%c'" RESET "\n", c);
 

    //Encriptacion del caractér:
     c^= key;

     
        printf(BRIGHT_MAGENTA "[Encriptado]" RESET " → Resultado: " YELLOW "'%c'" RESET "\n", c);
       

     //c^= key;
        
     //printf("Encriptado: %c\n", c);

    close(file_fd);

    if (nread != 1) {
        printf("[Emisor %d] Fin del archivo o error de lectura.\n", my_index);
        munmap(addr, total_size);
        close(fd);
        return 0;
    }

    time_t write_time = time(NULL);

    // Escribir carácter en su posición del buffer
    sem_wait(&buffer[my_index].sem_write);

    buffer[my_index].letter = c;
    buffer[my_index].index = my_index;
    buffer[my_index].time_stamp = write_time;
    sem_post(&buffer[my_index].sem_read);

    printf("\033[1;36m[Emisor %d]\033[0m → Escribió \033[1;33m'%c'\033[0m en buffer[\033[1;32m%d\033[0m]\n",
       my_index, c, my_index);
   

    
    
    printf(BRIGHT_RED"[Hora de escritura]-> %s", asctime(localtime(&write_time)));
    printf("\033[1;34m[======================================================]\033[0m\n");


    // Limpieza
    munmap(addr, total_size);
    close(fd);
    return 1;
  }

