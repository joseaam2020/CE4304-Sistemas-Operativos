#include "Conf_lect.h" // Aquí está la definición de struct conf y read_config()
#include "histograma.h"
#include <arpa/inet.h>
#include <dirent.h>
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

// Estructura para pasar múltiples argumentos al hilo
struct thread_args {
  int client_socket;
  struct conf cfg;
};

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

  //-------------------------entradas cerradas para que no interfieran--------
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  int fd = open("/dev/null", O_RDWR);
  dup(fd);
  dup(fd);
}

int get_next_image_number(const char *directory) {
  DIR *dir = opendir(directory);
  if (!dir)
    return 1; // Si no se puede abrir, empezamos desde 1

  struct dirent *entry;
  int max_number = 0;

  while ((entry = readdir(dir)) != NULL) {
    int num;
    if (sscanf(entry->d_name, "received_image_%d.jpg", &num) == 1) {
      if (num > max_number)
        max_number = num;
    }
  }

  closedir(dir);
  return max_number + 1;
}

void *handle_client(void *arg) {
  struct thread_args *args = (struct thread_args *)arg;
  int client_socket = args->client_socket;
  struct conf cfg = args->cfg;
  free(arg); // liberar memoria después de copiar los valores

  char image_path[MAX_PATH + 64];
  int next_id = get_next_image_number(cfg.histograma);
  snprintf(image_path, sizeof(image_path), "%s/received_image_%d.jpg",
           cfg.histograma, next_id);

  FILE *outfile = fopen(image_path, "wb");
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
  FILE *log =
      fopen(cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
  if (log) {
    time_t now = time(NULL);
    fprintf(log, "Imagen recibida: %d bytes - %s\n", total_received,
            ctime(&now));
    fclose(log);
  }

  // Clasificar la imagen
  const char *color = clasificar(image_path);
  if (color == NULL) {
    FILE *log = fopen(
        cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error clasificando la imagen %s\n", image_path);
      fclose(log);
    }
  } else {
    // Crear carpeta si no existe
    char new_dir[1024];
    snprintf(new_dir, sizeof(new_dir), "%s/%s", cfg.colores, color);

    struct stat st = {0};
    if (stat(new_dir, &st) == -1) {
      mkdir(new_dir, 0755);
    }

    // Construir ruta destino
    char new_path[1024];
    const char *filename = strrchr(image_path, '/');
    if (filename) {
      filename++;
    } else {
      filename = image_path;
    }

    snprintf(new_path, sizeof(new_path), "%s/%s", new_dir, filename);

    FILE *src = fopen(image_path, "rb");
    if (!src) {
      perror("No se pudo abrir la imagen original para copiar");
    } else {
      FILE *dest = fopen(new_path, "wb");
      if (!dest) {
        perror("No se pudo crear el archivo destino para la copia");
      } else {
        char copy_buffer[BUFFER_SIZE];
        size_t bytes;
        // Copia de imagen en directorio clasificado
        while ((bytes = fread(copy_buffer, 1, sizeof(copy_buffer), src)) > 0) {
          fwrite(copy_buffer, 1, bytes, dest);
        }
        fclose(dest);

        // Log si la copia fue exitosa
        FILE *log = fopen(
            cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
        if (log) {
          time_t now = time(NULL);
          fprintf(log, "Imagen copiada a %s - %s\n", new_path, ctime(&now));
          fclose(log);
        }
      }
      fclose(src);
    }
  }

  // Ecualizar histograma
  ecualizar_histograma_rgb(image_path);

  log =
      fopen(cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
  if (log) {
    time_t now = time(NULL);
    fprintf(log, "Ecualización aplicada a imagen: %s - %s\n", image_path,
            ctime(&now));
    fclose(log);
  }

  return NULL;
}

// Función principal del servidor socket
void start_server(struct conf cfg) {
  int port = cfg.port;
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Crear socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    FILE *log = fopen(
        cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
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
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    FILE *log = fopen(
        cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
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
    FILE *log = fopen(
        cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al setear SO_REUSEADDR\n");
      fclose(log);
    }
  }

  // Escuchar conexiones
  if (listen(server_socket, 5) < 0) {
    FILE *log = fopen(
        cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
    if (log) {
      fprintf(log, "Error al escuchar en puerto %d\n", port);
      fclose(log);
    }
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  //------------------------log de conexiones-----------------------------------
  FILE *log =
      fopen(cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
  if (log) {
    fprintf(log, "Servidor escuchando en puerto %d\n", port);
    fclose(log);
  }

  //------------------------ Bucle principal para aceptar conexiones ----------
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
      continue; // Continuar si hay error en accept
    }

    pthread_t thread_id;

    // Crear y rellenar la estructura de argumentos
    struct thread_args *args = malloc(sizeof(struct thread_args));
    args->client_socket = client_socket;
    args->cfg = cfg; // Se copia toda la estructura

    if (pthread_create(&thread_id, NULL, handle_client, args) != 0) {
      FILE *log = fopen(
          cfg.log_file[0] ? cfg.log_file : "/var/log/imageserver.log", "a+");
      if (log) {
        fprintf(log, "Error al crear hilo para cliente\n");
        fclose(log);
      }
      close(client_socket);
      free(args);
    } else {
      pthread_detach(
          thread_id); // El hilo se libera automáticamente al terminar
    }
  }

  close(server_socket);
}

//--------------------------------------------------------------------------------
int main() {
  // Leer configuración desde archivo
  struct conf cfg = read_config("/etc/ImagServer.conf");

  // Iniciar servidor con la configuración
  start_server(cfg);

  return 0;
}
