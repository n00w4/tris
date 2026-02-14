#ifndef NETWORK_H
#define NETWORK_H

#include "protocol.h"
#include <sys/socket.h>

extern int client_socket;

int connect_to_server(const char* ip, int port);
void disconnect_from_server(void);
int connect_with_config(void);
int send_message_to_server(const Message* msg);
int receive_message_from_server(Message* msg);

#endif
