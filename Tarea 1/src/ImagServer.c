#include "Conf_lect.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
int read_port_from_config(const char *filename);

//------------------------Definición del Daemon--------------------------------
void deamon() {
  pid_t process_id = fork();

  if (process_id < 0)
    exit(EXIT_FAILURE); // si el id es negativo error
  if (process_id > 0)
    exit(EXIT_SUCCESS); // si es positivo se muere el papa

  if (setsid() < 0)
    exit(EXIT_FAILURE);

  umask(0);
  chdir("/");

  //-------------------------entradas cerradas para que no
  // interfieran---------------------------------------------
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  int fd = open("/dev/null", O_RDWR);
  dup(fd);
  dup(fd);
}

void *handle_client(void *arg) {
  int client_socket = *(int *)arg;
  free(arg); // Liberar el puntero recibido

  FILE *outfile = fopen("received_image.jpg", "wb");
  if (!outfile) {
    perror("No se pudo crear el archivo de imagen");
    close(client_socket);
    return NULL;
  }

  int block_size;
  char buffer[BUFFER_SIZE];
  int total_received = 0;

  while (1) {
    // Leer tamaño del bloque (4 bytes)
    int net_block_size;
    int read_bytes =
        recv(client_socket, &net_block_size, sizeof(net_block_size), 0);
    if (read_bytes <= 0) {
      perror("Error leyendo el tamaño del bloque");
      break;
    }

    block_size = ntohl(net_block_size); // Convertir a orden de bytes del host

    if (block_size == 0) {
      // Fin de transmisión
      break;
    }

    // Leer los datos del bloque
    int bytes_remaining = block_size;
    while (bytes_remaining > 0) {
      int chunk_size =
          bytes_remaining > BUFFER_SIZE ? BUFFER_SIZE : bytes_remaining;
      int received = recv(client_socket, buffer, chunk_size, 0);
      if (received <= 0) {
        perror("Error leyendo los datos del bloque");
        fclose(outfile);
        close(client_socket);
        return NULL;
      }

      fwrite(buffer, 1, received, outfile);
      total_received += received;
      bytes_remaining -= received;
    }
  }

  fclose(outfile);
  close(client_socket);

  // Registrar en el log
  FILE *log = fopen("/var/log/imageserver.log", "a+");
  if (log) {
    time_t now = time(NULL);
    fprintf(log, "Imagen recibida: %d bytes - %s\n", total_received,
            ctime(&now));
    fclose(log);
  }

  printf("Imagen recibida correctamente (%d bytes)\n", total_received);
  return NULL;
}

// Función principal del servidor socket
void start_server() {
  int port = read_port_from_config("/etc/ImagServer.conf");
  printf("DEBUG: Puerto leído: %d\n", port);
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Crear socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    FILE *log = fopen("/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al crear socket\n");
      fclose(log);
    }
    exit(EXIT_FAILURE);
  }

  // Configurar dirección del servidor
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  // Ligar socket al puerto
  printf("DEBUG: Puerto que se va a bind: %d\n", ntohs(server_addr.sin_port));
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    FILE *log = fopen("/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al hacer bind en puerto %d\n", port);
      fclose(log);
    }
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    FILE *log = fopen("/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al setear SO_REUSEADDR\n");
      fclose(log);
    }
  }

  // Escuchar conexiones
  if (listen(server_socket, 5) < 0) {
    FILE *log = fopen("/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al escuchar en puerto %d\n", port);
      fclose(log);
    }
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  //------------------------log de conexiones-----------------------------------
  FILE *log = fopen("/var/log/imageserver.log", "a+");
  if (log) {
    fprintf(log, "Servidor escuchando en puerto %d\n", port);
    fclose(log);
  }

  //------------------------ Bucle principal para aceptar conexiones
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
      continue; // Continuar si hay error en accept
    }

    // Crear hilo para manejar el cliente
    pthread_t thread_id;
    int *client_sock_ptr = malloc(sizeof(int));
    *client_sock_ptr = client_socket;

    if (pthread_create(&thread_id, NULL, handle_client, client_sock_ptr) != 0) {
      FILE *log = fopen("/var/log/imageserver.log", "a+");
      if (log) {
        fprintf(log, "Error al crear hilo para cliente\n");
        fclose(log);
      }
      close(client_socket);
      free(client_sock_ptr);
    } else {
      pthread_detach(
          thread_id); // El hilo se libera automáticamente al terminar
    }
  }

  close(server_socket);
}

//--------------------------------------------------------------------------------
int main() {
  start_server();

  return 0;
}
