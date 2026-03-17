#include "server.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int sig) {
  (void)sig;
  keep_running = 0;
}

int main(void) {
  signal(SIGPIPE, SIG_IGN);

  // signal handlers for graceful shutdown
  struct sigaction sa = {0};
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction SIGINT");
    return 1;
  }
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    perror("sigaction SIGTERM");
    return 1;
  }

  initialize_server();

  int server_socket = create_server_socket(SERVER_PORT);
  if (server_socket < 0) {
    fprintf(stderr, "Failed to create server\n");
    cleanup_server();
    return 1;
  }

  printf("Waiting for connections... (Ctrl+C to stop)\n");

  while (keep_running) {
    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
      if (errno == EINTR && !keep_running) {
        break;
      }
      perror("accept error");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    int* socket_ptr = malloc(sizeof(int));
    if (socket_ptr == NULL) {
      perror("malloc failed");
      close(client_socket);
      continue;
    }
    *socket_ptr = client_socket;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, socket_ptr) != 0) {
      perror("pthread_create failed");
      free(socket_ptr);
      close(client_socket);
      continue;
    }
    pthread_detach(thread_id);
  }

  printf("\nShutting down server...\n");

  // close server socket to prevent new connections
  close(server_socket);

  // notify all active clients by closing their sockets
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active) {
      printf("[main] Closing client socket %d\n", clients[i].socket);
      close(clients[i].socket);
      clients[i].is_active = false;
      clients[i].current_game_id = -1;
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L }; // 100ms
  int waited_ms = 0;
  while (atomic_load(&active_thread_count) > 0 && waited_ms < 5000) {
    nanosleep(&ts, NULL);
    waited_ms += 100;
  }
  if (atomic_load(&active_thread_count) > 0) {
    fprintf(stderr, "[main] Warning: %d thread(s) still active after timeout\n", atomic_load(&active_thread_count));
  }

  cleanup_server();
  printf("Server terminated.\n");
  return 0;
}
