#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <stdint.h>

struct Game;

/**
 * Handles the request of the list of games 
*/
void handle_list_games(int client_socket);

#endif
