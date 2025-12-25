#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 100
#define MAX_GAMES 50
#define BUFFER_SIZE 1024
#define SERVER_PORT 8080

typedef struct Client {
  int socket;
  char username[50];
  int current_game_id;
  bool is_playing;
  struct Client* next;
} Client;

typedef struct Game Game;

extern Client* clients;
extern Game* games;
extern int client_count;
extern int game_count;

extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t games_mutex;


/**
 * Initialize the global state of the server
 */
void initialize_server();

/**
 * Cleanup server resources
 */
void cleanup_server();

/**
 * Create and configure server's socket
 */
int create_server_socket(int port);

/**
 * Handle a single client in thread level
 */
void* handle_client(void* arg);

#endif
