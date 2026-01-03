#include "protocol.h"

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int send_message(int socket, const Message* msg) {
  if (msg == NULL) {
    fprintf(stderr, "[protocol] send_message: message is NULL\n");
    return -1;
  }

  size_t total_sent = 0;
  size_t msg_size = sizeof(Message);
  const char* buffer = (const char*) msg;

  while (total_sent < msg_size) {
    ssize_t sent = send(socket, buffer + total_sent, msg_size - total_sent, 0);

    if (sent < 0) {
      fprintf(stderr, "[protocol] Failed after sending %zu/%zu bytes\n", total_sent, msg_size);
      return -1;
    }

    total_sent += sent;
  }
    return 0;
}

int receive_message(int socket, Message* msg) {
  if (msg == NULL) {
    fprintf(stderr, "[protocol] receive_message: message is NULL\n");
    return -1;
  }

  size_t total_received = 0;
  size_t msg_size = sizeof(Message);
  char* buffer = (char*) msg;

  memset(msg, 0, msg_size);
  
  while (total_received < msg_size) {
    ssize_t received = recv(socket, buffer + total_received, msg_size - total_received, 0);

    if (received < 0) {
      fprintf(stderr, "[protocol] Failed after receiving %zu/%zu bytes\n", total_received, msg_size);
      return -1;
    }

    total_received += received;
  }

  if (msg->type < MSG_CREATE_GAME || msg->type > MSG_ERROR) {
    fprintf(stderr, "[protocol] Invalid message type: %d\n", msg->type);
    return -1;
  }
  
  return 0;
}

