#include "network.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

int client_socket = -1;

int connect_to_server(const char* ip, int port) {
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    close(client_socket);
    return -1;
  }

  printf("Connected to server.\n");
  return 0;
}

void disconnect_from_server(void) {
  if (client_socket >= 0) {
    close(client_socket);
    client_socket = -1;
  }
}

int send_message_to_server(const Message* msg) {
  if (client_socket < 0) {
    fprintf(stderr, "[network] Not connected to server.\n");
    return -1;
  }

  return send_message(client_socket, msg);
}

int receive_message_from_server(Message* msg) {
  if (client_socket < 0) {
    fprintf(stderr, "[network] Not connected to server.\n");
    return -1;
  }

  return receive_message(client_socket, msg);
}
