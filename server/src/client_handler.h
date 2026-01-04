#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "protocol.h"

struct Game;


/**
 * Handles requests
 */
void handle_message(int client_socket, Message* msg);

/**
 * Handles the request of the list of games 
*/
void handle_list_games(int client_socket);

#endif
