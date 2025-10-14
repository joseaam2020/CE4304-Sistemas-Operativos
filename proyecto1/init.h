#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <time.h>

// Estructura principal de la tabla compartida
struct SharedTable {
  // Posicion de lectura emisores
  unsigned long read_pos;
  sem_t sem_read_pos;

  // Evento para finalizar
  short end_program;
  sem_t sem_end_program;

  // Indice para saber siguiente posicion en buffer (Emisores)
  int emiter_index;
  sem_t sem_emiter_index;

  // Indice para saber siguiente posicion en buffer (Receptores)
  int receptor_index;
  sem_t sem_receptor_index;

  int buffer_size;

  // Senial para finalizar el programa
  short finalizado;
  sem_t sem_finalizado;
};

// Estructura de cada posición del buffer
struct BufferPosition {
  char letter;
  sem_t sem_read;
  sem_t sem_write;
  int index;
  time_t time_stamp;
};

// Layout completo en memoria compartida:
// [SharedTable][BufferPosition array][file_path string]

#endif // SHARED_H
