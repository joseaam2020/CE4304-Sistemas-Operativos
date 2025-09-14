#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BLOCK_SIZE 1024
#define SA struct sockaddr

// Función para enviar una imagen dividida en bloques
void send_image(int sockfd, const char *filename) {
  printf("Imagen: %s\n", filename);
  FILE *file = fopen(filename, "rb");
  if (!file) {
    perror("Error opening file");
    return;
  }

  char buffer[MAX_BLOCK_SIZE];
  int bytes_read;

  while ((bytes_read = fread(buffer, 1, MAX_BLOCK_SIZE, file)) > 0) {
    // Enviar tamaño del bloque primero (en formato de red)
    int net_bytes = htonl(bytes_read);
    if (write(sockfd, &net_bytes, sizeof(net_bytes)) != sizeof(net_bytes)) {
      perror("Failed to send block size");
      break;
    }

    // Enviar contenido del bloque
    if (write(sockfd, buffer, bytes_read) != bytes_read) {
      perror("Failed to send block data");
      break;
    }
  }

  // Enviar bloque de tamaño 0 para indicar fin de archivo
  int zero = 0;
  zero = htonl(zero);
  write(sockfd, &zero, sizeof(zero));

  fclose(file);
  printf("Image '%s' sent successfully.\n", filename);
}

int main() {
  int sockfd;
  struct sockaddr_in servaddr;
  char ip_address[100];
  int port;
  char filename[256];
  char image_name[200];

  // Solicitar IP y puertr
  printf("Enter IP address of server (e.g., 127.0.0.1): ");
  scanf("%s", ip_address);

  printf("Enter port number (e.g., 1717): ");
  scanf("%d", &port);

  // Solicitar nombre del archivo
  printf("Enter image filename (e.g., photo.jpg): ");
  scanf("%s", image_name);

  // Crear la ruta completa con el prefijo "imgs/"
  snprintf(filename, sizeof(filename), "imgs/%s", image_name);
  char cwd[PATH_MAX];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("Directorio actual: %s\n", cwd);
  } else {
    perror("getcwd() error");
    return 1;
  }

  // Crear socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip_address);
  servaddr.sin_port = htons(port);

  // Conectar al servidor
  printf("Connecting to %s:%d...\n", ip_address, port);
  if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
    perror("Connection failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Connected to the server.\n");

  // Enviar la imagen
  send_image(sockfd, filename);

  // Cerrar la conexión
  close(sockfd);
  return 0;
}
