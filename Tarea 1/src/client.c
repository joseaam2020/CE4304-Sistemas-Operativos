#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX 1024
#define SA struct sockaddr

// Función que maneja la comunicación con el servidor
void func(int sockfd) {
  char buff[MAX];
  for (;;) {
    // Leer entrada del usuario
    bzero(buff, sizeof(buff));
    printf("Enter the string: ");
    if (fgets(buff, MAX, stdin) == NULL) {
      printf("Input error. Exiting...\n");
      break;
    }
    buff[strcspn(buff, "\n")] = 0; // Quitar salto de línea

    // Enviar mensaje al servidor
    if (write(sockfd, buff, strlen(buff)) < 0) {
      perror("Write failed");
      break;
    }

    // Recibir respuesta del servidor
    bzero(buff, sizeof(buff));
    int bytes = read(sockfd, buff, sizeof(buff) - 1);
    if (bytes <= 0) {
      perror("Read failed or connection closed");
      break;
    }
    buff[bytes] = '\0';
    printf("From Server: %s\n", buff);

    // Salir si se recibe el mensaje de confirmación
    if (strncmp(buff, "Servidor: Mensaje recibido correctamente", 41) == 0) {
      printf("Client Exit...\n");
      break;
    }
  }
}

int main() {
  int sockfd;
  struct sockaddr_in servaddr;
  char ip_address[100];
  int port;

  // Solicitar IP y puerto al usuario
  printf("Enter IP address of server (e.g., 127.0.0.1): ");
  scanf("%s", ip_address);

  printf("Enter port number (e.g., 1717): ");
  scanf("%d", &port);
  getchar(); // Captura el salto de línea restante

  // Crear socket TCP
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Configurar dirección del servidor
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip_address);
  servaddr.sin_port = htons(port);

  printf("Connecting to %s:%d...\n", ip_address, port);

  // Intentar conectarse al servidor (con reintentos)
  int retries = 5;
  while (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0 && retries--) {
    printf("Connection failed. Retrying...\n");
    sleep(1);
  }

  if (retries <= 0) {
    perror("Connection with the server failed after retries");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Connected to the server.\n");

  // Iniciar comunicación
  func(sockfd);

  // Cerrar socket al terminar
  close(sockfd);
}
