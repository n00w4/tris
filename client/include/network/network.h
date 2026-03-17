#ifndef NETWORK_H
#define NETWORK_H

#include "utils/queue.h"

#include <stdbool.h>
#include <arpa/inet.h>

typedef struct network_ctx network_ctx;

network_ctx* network_start(const char* ip, int port, queue_t* to_ui, queue_t* from_ui);
void network_stop(network_ctx* ctx);
void network_wait(network_ctx* ctx);
void network_destroy(network_ctx* ctx);
bool network_is_connected(network_ctx* ctx);

#endif
