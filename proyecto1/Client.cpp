#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

//#include "shared.h" include del .h del archivo de la tabla
 
#define SHM_NAME ""
#define SHM_SIZE sizeof()

int main(void){
    int fd = shm_open(SHM_NAME,O_RDONLY,0);
    int (fd == -1 ){ perror("shm_open"); return 1; }

    shared_table_t *table = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    printf("file_path: '%s'\n", table->file_path);
    printf("buffer_len: %zu, read_pos: %zu, index: %zu\n",
           table->buffer_len, table->read_pos, table->index);

    munmap(table, SHM_SIZE);
    close(fd);
    return 0;


}
