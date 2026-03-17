#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "game.h"
#include "protocol.h"
#include "client_handler.h"

Client clients[MAX_CLIENTS];
static int active_clients_count = 0; 
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

atomic_int active_thread_count = 0;

GameManager *game_manager = NULL;

Client* add_client(int client_socket) {
  if (active_clients_count >= MAX_CLIENTS) {
    fprintf(stderr, "[server] Server full! Max clients: %d\n", MAX_CLIENTS);
    return NULL;
  }
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i].is_active) {
      clients[i].socket = client_socket;
      clients[i].is_active = true;
      clients[i].is_playing = false;
      clients[i].current_game_id = -1;
      snprintf(clients[i].username, sizeof(clients[i].username), "Guest");
      active_clients_count++;
      printf("[server] New client connected (slot %d, socket %d). Total: %d\n",
          i, client_socket, active_clients_count);
      return &clients[i];
    }
  }
  return NULL;
}

void remove_client(int client_socket) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == client_socket) {
      printf("[server] Client removed (slot %d, socket %d)\n", i, client_socket);
      clients[i].is_active = false;
      clients[i].is_playing = false;
      clients[i].current_game_id = -1;
      active_clients_count--;
      return;
    }
  }
}

int server_get_active_clients(void) {
  pthread_mutex_lock(&clients_mutex);
  int count = active_clients_count;
  pthread_mutex_unlock(&clients_mutex);
  return count;
}

void initialize_server(void) {
  printf("[server] Initializing...\n");
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].is_active = false;
  }
  active_clients_count = 0;
  game_manager = game_manager_create();
  if (!game_manager) {
    fprintf(stderr, "[server] FATAL: Failed to create GameManager\n");
    exit(EXIT_FAILURE);
  }
}

void cleanup_server(void) {
  if (game_manager) {
    game_manager_destroy(game_manager);
    game_manager = NULL;
  }
  pthread_mutex_destroy(&clients_mutex);
  printf("[server] Cleanup completed\n");
}

int create_server_socket(int port) {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("[server] socket failed");
    return -1;
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("[server] setsockopt SO_REUSEADDR failed");
    close(server_fd);
    return -1;
  }

#ifdef SO_REUSEPORT
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    perror("[server] setsockopt SO_REUSEPORT (non‑fatal)");
    // continue anyway
  }
#endif

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("[server] bind failed");
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, BACKLOG) < 0) {
    perror("[server] listen failed");
    close(server_fd);
    return -1;
  }

  printf("[server] Listening on port %d\n", port);
  return server_fd;
}

void* handle_client(void* arg) {
  atomic_fetch_add(&active_thread_count, 1);

  int client_socket = *(int*)arg;
  free(arg);

  // Add client to active list
  pthread_mutex_lock(&clients_mutex);
  Client* client = add_client(client_socket);
  pthread_mutex_unlock(&clients_mutex);

  if (!client) {
    // Server full
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "%s", MAX_CONN_MSG);
    send_message(client_socket, &err);
    close(client_socket);

    atomic_fetch_sub(&active_thread_count, 1);
    return NULL;
  }

  // Main message loop
  Message msg;
  while (1) {
    memset(&msg, 0, sizeof(msg));
    int ret = receive_message(client_socket, &msg);
    if (ret == -2) {
      printf("[server] Client socket %d closed connection (EOF)\n", client_socket);
      break;
    }
    if (ret == -1) {
      fprintf(stderr, "[server] receive_message error on socket %d: %s\n",
          client_socket, strerror(errno));
      break;
    }
    handle_message(client_socket, &msg);
  }

  int game_id = -1;
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == client_socket) {
      game_id = clients[i].current_game_id;
      break;
    }
  }
  remove_client(client_socket);
  pthread_mutex_unlock(&clients_mutex);

  if (game_id != -1) {
    game_manager_leave_game(game_manager, (uint32_t)game_id, client_socket);
  }
  close(client_socket);

  atomic_fetch_sub(&active_thread_count, 1);
  return NULL;
}

void find_username_by_socket(int socket, char *username_output, size_t out_size) {
  if (!username_output || out_size == 0) return;
  pthread_mutex_lock(&clients_mutex);
  int found = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == socket) {
      strncpy(username_output, clients[i].username, out_size - 1);
      username_output[out_size - 1] = '\0';
      found = 1;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  if (!found) {
    strncpy(username_output, "Unknown", out_size - 1);
    username_output[out_size - 1] = '\0';
  }
}
