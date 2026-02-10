#ifndef SERVER_H
#define SERVER_H

#include "game.h"

#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 100
#define MAX_GAMES 50
#define BUFFER_SIZE 1024
#define SERVER_PORT 8080
#define BACKLOG 10
#define MAX_CONN_MSG "Full server. Try again later.\n"

typedef struct Game Game;

typedef struct Client {
  int socket;
  char username[50];
  int current_game_id;
  bool is_playing;
  bool is_active;
} Client;

extern Client clients[MAX_CLIENTS];
extern Game* games;
extern int active_clients_count;
extern int game_count;

extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t games_mutex;


/**
 * Add client to the array
 */
Client* add_client(int client_socket);

/**
 * Remove a client from the array
 */
void remove_client(int client_socket);

/**
 * Initialize the global state of the server
 */
void initialize_server(void);

/**
 * Cleanup server resources
 */
void cleanup_server(void);

/**
 * Create and configure server's socket
 */
int create_server_socket(int port);

/**
 * Handle a single client in thread level
 */
void* handle_client(void* arg);

/**
 * Add a game to the list
 */
void add_game_to_list(Game* new_game);

#endif
