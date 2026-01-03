#include "server.h"
#include "game.h"

Client clients[MAX_CLIENTS];
Game* games = NULL;
int active_clients_count = 0;
int game_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

Client* add_client(int client_socket) {
  if (active_clients_count >= MAX_CLIENTS) {
    fprintf(stderr, "Server full! Max clients: %d\n", MAX_CLIENTS);
    return NULL;
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i].is_active) {
      clients[i].socket = client_socket;
      clients[i].is_active = true;
      clients[i].is_playing = false;
      clients[i].current_game_id = -1;
      strcpy(clients[i].username, "Guest");

      active_clients_count++;

      printf("[+] New client connected (slot %d, socket %d). Total: %d\n", i, client_socket, active_clients_count);
      return &clients[i];
    }
  }
  return NULL; 
}

void remove_client(int client_socket) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == client_socket) {
      printf("[-] Client disconnected (slot %d, socket %d)\n", i, client_socket);
      close(client_socket);

      clients[i].is_active = false;
      clients[i].is_playing = false;
      clients[i].current_game_id = -1;

      active_clients_count--;
      return;
    }
  }
}

void initialize_server() {
  printf("Server initialized\n");
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].is_active = false;
  }
  games = NULL;
  active_clients_count = 0;
  game_count = 0;
}

void cleanup_server() {
  pthread_mutex_lock(&games_mutex);
  Game* current = games;
  while (current != NULL) {
    Game* next = current->next;
    free(current);
    current = next;
  }
  games = NULL;
  pthread_mutex_unlock(&games_mutex);

  pthread_mutex_destroy(&clients_mutex);
  pthread_mutex_destroy(&games_mutex);

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

  printf("[server] New client thread started (socket %d)\n", client_socket);

  pthread_mutex_lock(&clients_mutex);
  Client* client = add_client(client_socket);
  pthread_mutex_unlock(&clients_mutex);

  if (client == NULL) {
    close(client_socket);
    return NULL;
    // TODO: send a message to a client who can't connect
  }

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

    buffer[strcspn(buffer, "\n\r")] = 0;
    printf("Received from socket %d: %s\n", client_socket, buffer);

    ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
    if (bytes_sent < 0) {
      perror("send error");
      break;
    }
  }

  pthread_mutex_lock(&clients_mutex);
  remove_client(client_socket);
  pthread_mutex_unlock(&clients_mutex);

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
