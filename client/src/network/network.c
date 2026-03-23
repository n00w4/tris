#include "network/network.h"
#include "ui/ui_events.h"
#include "protocol.h"
#include "utils/queue.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>

struct network_ctx {
  atomic_int sock;
  queue_t* to_ui;
  queue_t* from_ui;
  pthread_t thread;
  atomic_int running;
  char ip[INET_ADDRSTRLEN];
  int port;
};

static UIEvent* message_to_event(const Message* msg) {
  UIEvent* ev = malloc(sizeof(UIEvent));
  if (!ev) { return NULL; }

  switch (msg->type) {
    case MSG_GAME_STATE:
      ev->type = UI_EVENT_GAME_STATE;
      ev->data.game_state = msg->payload.game_state;
      break;
    case MSG_GAME_START:
      ev->type = UI_EVENT_GAME_START;
      ev->data.game_start = msg->payload.game_start;
      break;
    case MSG_GAME_OVER:
      ev->type = UI_EVENT_GAME_OVER;
      ev->data.game_over = msg->payload.game_over;
      break;
    case MSG_LOBBY_UPDATE:
      ev->type = UI_EVENT_LOBBY_UPDATE;
      ev->data.lobby_update = msg->payload.lobby_update;
      break;
    case MSG_ERROR:
      ev->type = UI_EVENT_ERROR;
      ev->data.error = msg->payload.error;
      break;
    case MSG_JOIN_REQUEST:
      ev->type = UI_EVENT_JOIN_REQUEST;
      ev->data.join_request = msg->payload.join_request;
      break;
    case MSG_JOIN_DECISION:
      ev->type = UI_EVENT_JOIN_DECISION;
      ev->data.join_decision = msg->payload.join_decision;
      break;
    case MSG_POST_GAME_OPTIONS:
      ev->type = UI_EVENT_POST_GAME_OPTIONS;
      ev->data.post_game_options = msg->payload.post_game_options;
      break;
    case MSG_GAME_STATUS_CHANGE:
      ev->type = UI_EVENT_GAME_STATUS_CHANGE;
      ev->data.game_status_change = msg->payload.status_change;
      break;
    default:
      free(ev);
      return NULL;
  }
  return ev;
}

static void drain_message_queue(queue_t* q) {
  void* item;
  while (queue_try_pop(q, &item) == 0) {
    free(item);
  }
}

static void thread_exit_error(network_ctx* ctx, const char* user_msg) {
  UIEvent* ev = malloc(sizeof(UIEvent));
  if (ev) {
    ev->type = UI_EVENT_ERROR;
    snprintf(ev->data.error.error_message,
             sizeof(ev->data.error.error_message),
             "%s", user_msg);
    queue_push(ctx->to_ui, ev);
  }
  int old_fd = atomic_exchange(&ctx->sock, -1);
  if (old_fd >= 0) { close(old_fd); }
  atomic_store(&ctx->running, 0);
}

static void* network_thread_func(void* arg) {
  network_ctx* ctx = arg;

  // create socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    thread_exit_error(ctx, "Socket creation failed");
    return NULL;
  }
  atomic_store(&ctx->sock, fd);

  // connect to the server
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)ctx->port);
  if (inet_pton(AF_INET, ctx->ip, &addr.sin_addr) <= 0) {
    thread_exit_error(ctx, "Invalid IP address");
    return NULL;
  }

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    thread_exit_error(ctx, "Connection to server failed");
    return NULL;
  }

  // main loop
  while (atomic_load(&ctx->running)) {
    // pop ui queue
    Message* cmd;
    while (queue_try_pop(ctx->from_ui, (void**)&cmd) == 0) {
      if (send_message(fd, cmd) < 0) {
        perror("[network] send_message");
        UIEvent* ev = malloc(sizeof(UIEvent));
        if (ev) {
          ev->type = UI_EVENT_ERROR;
          snprintf(ev->data.error.error_message,
                   sizeof(ev->data.error.error_message),
                   "Send failed, disconnecting");
          queue_push(ctx->to_ui, ev);
        }
        atomic_store(&ctx->running, 0);
        free(cmd);
        break;
      }
      free(cmd);
    }
    if (!atomic_load(&ctx->running)) { break; }

    // wait for data from socket (100 ms)
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int pret = poll(&pfd, 1, 100);
    if (pret < 0) {
      if (errno == EINTR) { continue; }
      perror("[network] poll");
      break;
    }
    if (pret > 0 && (pfd.revents & POLLIN)) {
      Message msg = {0};
      int r = receive_message(fd, &msg);
      if (r == -2) {   // connection closed from server
        UIEvent* ev = malloc(sizeof(UIEvent));
        if (ev) {
          ev->type = UI_EVENT_DISCONNECTED;
          queue_push(ctx->to_ui, ev);
        }
        break;
      } else if (r < 0) {
        perror("[network] receive_message");
        UIEvent* ev = malloc(sizeof(UIEvent));
        if (ev) {
          ev->type = UI_EVENT_ERROR;
          snprintf(ev->data.error.error_message,
                   sizeof(ev->data.error.error_message),
                   "Receive error");
          queue_push(ctx->to_ui, ev);
        }
        break;
      }
      UIEvent* ev = message_to_event(&msg);
      if (!ev) {
        UIEvent* err_ev = malloc(sizeof(UIEvent));
        if (err_ev) {
          err_ev->type = UI_EVENT_ERROR;
          snprintf(err_ev->data.error.error_message,
                   sizeof(err_ev->data.error.error_message),
                   "Out of memory processing server message");
          queue_push(ctx->to_ui, err_ev);
        }
        fprintf(stderr, "[network] OOM: dropped message type %d\n", msg.type);
      } else {
        queue_push(ctx->to_ui, ev);
      }
    }
  }

  drain_message_queue(ctx->from_ui);
  int old_fd = atomic_exchange(&ctx->sock, -1);
  if (old_fd >= 0) { close(old_fd); }
  atomic_store(&ctx->running, 0);
  return NULL;
}

network_ctx* network_start(const char* ip, int port, queue_t* to_ui, queue_t* from_ui) {
  if (!ip || !to_ui || !from_ui) { return NULL; }

  network_ctx* ctx = malloc(sizeof(network_ctx));
  if (!ctx) { return NULL; }

  strncpy(ctx->ip, ip, sizeof(ctx->ip) - 1);
  ctx->ip[sizeof(ctx->ip) - 1] = '\0';
  ctx->port = port;
  atomic_init(&ctx->sock, -1);
  ctx->to_ui = to_ui;
  ctx->from_ui = from_ui;
  atomic_init(&ctx->running, 1);

  if (pthread_create(&ctx->thread, NULL, network_thread_func, ctx) != 0) {
    perror("[network] pthread_create");
    free(ctx);
    return NULL;
  }

  return ctx;
}

void network_stop(network_ctx* ctx) {
  if (!ctx) { return; }
  atomic_store(&ctx->running, 0);
  int fd = atomic_load(&ctx->sock);
  if (fd >= 0) {
    shutdown(fd, SHUT_RDWR);
  }
}

void network_wait(network_ctx* ctx) {
  if (!ctx) { return; }
  pthread_join(ctx->thread, NULL);
}

void network_destroy(network_ctx* ctx) {
  if (!ctx) { return; }
  free(ctx);
}

bool network_is_connected(network_ctx* ctx) {
  if (!ctx) { return false; }
  int fd = atomic_load(&ctx->sock);
  return (fd >= 0 && atomic_load(&ctx->running));
}
