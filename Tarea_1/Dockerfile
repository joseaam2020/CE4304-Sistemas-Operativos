# Dockerfile
FROM gcc:latest

WORKDIR /app

COPY src/client.c ./client.c

RUN gcc -o client client.c

# Ejecuta el binario directamente (esperará input del usuario)
ENTRYPOINT ["./client"]
