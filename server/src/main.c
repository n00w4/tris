#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "server.h"

int main() {
  initialize_server();

  int server_socket;
  int client_socket;
  struct sockaddr_in client_address;
  socklen_t client_len = sizeof(client_address);
  pthread_t thread_id;

  server_socket = create_server_socket(SERVER_PORT);
  if (server_socket < 0) {
    fprintf(stderr, "Failed to create server\n");
    return 1;
  }

  printf("Waiting for connections...\n");

  while (1) {
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
    if (client_socket < 0) {
      perror("accept error");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Connection from %s:%d\n", client_ip, ntohs(client_address.sin_port));

    int* socket_ptr = malloc(sizeof(int));
    if (socket_ptr == NULL) {
      perror("malloc failed");
      close(client_socket);
      continue;
    }

    *socket_ptr = client_socket;

    if (pthread_create(&thread_id, NULL, handle_client, socket_ptr) != 0) {
      perror("pthread_create failed");
      free(socket_ptr);
      close(client_socket);
      continue;
    }

    pthread_detach(thread_id);
  }

  cleanup_server();

  close(server_socket);
  return 0;
}
