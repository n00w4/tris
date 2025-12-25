#include "server.h"
#include "game.h"

#define BACKLOG 10
#define MAX_CONN_MSG "Full server. Try again later.\n"


Client* clients = NULL;
Game* games = NULL;
int client_count = 0;
int game_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;


void initialize_server() {
  printf("Server initialized\n");
  clients = NULL;
  games = NULL;
  client_count = 0;
  game_count = 0;
}

void cleanup_server() {
  // TODO: Implement cleanup
  printf("Server cleanup completed\n");
}

int create_server_socket(int port) {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket failed");
    return -1;
  }
    
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    close(server_fd);
    return -1;
  }
    
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
    
  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("bind failed");
    close(server_fd);
    return -1;
  }
    
  if (listen(server_fd, BACKLOG) < 0) {
    perror("listen failed");
    close(server_fd);
    return -1;
  }
    
  printf("Server listening on port %d\n", port);
  return server_fd;
}

void* handle_client(void* arg) {
  int client_socket = *(int*)arg;
  free(arg);
    
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
    
  printf("New client connected (socket %d)\n", client_socket);
    
  while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
    if (bytes_read <= 0) {
      if (bytes_read == 0) {
        printf("Client disconnected (socket %d)\n", client_socket);
      } else {
        perror("recv error");
      }
      break;
    }
        
    printf("Received from socket %d: %s", client_socket, buffer);
        
    ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
    if (bytes_sent < 0) {
      perror("send error");
      break;
    }
  }

  close(client_socket);
  printf("Closed socket %d\n", client_socket);
    
  return NULL;
}

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
