#include "client_handler.h"
#include "game.h"
#include "protocol.h"
#include "server.h"

#include <stdio.h>
#include <string.h>

extern GameManager *game_manager;

static void build_lobby_message(Message *out_msg, const GameSnapshot *snapshots, uint8_t count) {
  memset(out_msg, 0, sizeof(Message));
  out_msg->type = MSG_LOBBY_UPDATE;
  out_msg->payload.lobby_update.game_count = count;
  for (int i = 0; i < count; i++) {
    out_msg->payload.lobby_update.games[i].game_id = snapshots[i].id;
    out_msg->payload.lobby_update.games[i].state = snapshots[i].state;
    strncpy(out_msg->payload.lobby_update.games[i].owner_name,
        snapshots[i].owner_name,
        sizeof(out_msg->payload.lobby_update.games[i].owner_name)-1);
    
    out_msg->payload.lobby_update.games[i].owner_name[
      sizeof(out_msg->payload.lobby_update.games[i].owner_name)-1] = '\0';
    out_msg->payload.lobby_update.games[i].players_connected = snapshots[i].players_connected;
  }
  printf("%ld\n", (long)out_msg->payload.lobby_update.games[0].game_id);
}

static bool client_is_playing(int client_socket) {
  pthread_mutex_lock(&clients_mutex);
  bool playing = false;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == client_socket &&
        clients[i].is_playing) {
      playing = true;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return playing;
}

static void client_set_playing(int socket, bool playing, uint32_t game_id) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == socket) {
      clients[i].is_playing = playing;
      clients[i].current_game_id = playing ? (int)game_id : -1;
      printf("[server] client_set_playing: socket %d, playing=%d, game_id=%d\n",
          socket, playing, playing ? (int)game_id : -1);
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void broadcast_lobby_update(void) {
  GameSnapshot snapshots[10];
  uint8_t count;
  game_manager_list_games(game_manager, snapshots, &count);

  Message lobby_msg;
  build_lobby_message(&lobby_msg, snapshots, count);

  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && !clients[i].is_playing) {
      send_message(clients[i].socket, &lobby_msg);
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void handle_message(int client_socket, Message* msg) {
  switch (msg->type) {
    case MSG_CREATE_GAME:
      handle_create_game(client_socket);
      break;
    case MSG_JOIN_GAME:
      handle_join_request(client_socket, msg->payload.join_request.game_id);
      break;
    case MSG_JOIN_DECISION:
      handle_join_decision(client_socket,
          msg->payload.join_decision.accepted,
          msg->payload.join_decision.game_id);
      break;
    case MSG_POST_GAME_OPTIONS:
      handle_post_game_decision(client_socket,
          msg->payload.post_game_options.game_id,
          msg->payload.post_game_options.winner_wants_to_continue);
      break;
    case MSG_MOVE:
      handle_move(client_socket, msg);
      break;
    case MSG_LIST_GAMES:
      handle_list_game(client_socket);
      break;
    case MSG_LEAVE_GAME:
      handle_leave_game(client_socket, msg->payload.move.game_id);
      break;
    default: {
               Message err;
               err.type = MSG_ERROR;
               snprintf(err.payload.error.error_message,
                   sizeof(err.payload.error.error_message),
                   "Unsupported message type: %d", msg->type);
               send_message(client_socket, &err);
               break;
             }
  }
}

void handle_list_game(int client_socket) {
  GameSnapshot snapshots[10];
  uint8_t count;
  game_manager_list_games(game_manager, snapshots, &count);

  Message resp;
  build_lobby_message(&resp, snapshots, count);

  printf("[Handler] Sending lobby update to socket %d (%d games)\n",
      client_socket, count);

  if (send_message(client_socket, &resp) < 0) {
    fprintf(stderr, "[Handler] Failed to send lobby update to socket %d\n", client_socket);
  }
}

void handle_create_game(int client_socket) {
  if (client_is_playing(client_socket)) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Already in a game");
    send_message(client_socket, &err);
    return;
  }

  char username[32] = "Unknown";
  find_username_by_socket(client_socket, username, sizeof(username));

  uint32_t new_game_id;
  if (game_manager_create_game(game_manager, client_socket, username,
        &new_game_id) < 0) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Failed to create game");
    send_message(client_socket, &err);
    return;
  }

  client_set_playing(client_socket, true, new_game_id);

  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].is_active && clients[i].socket == client_socket) {
      clients[i].is_playing = true;
      clients[i].current_game_id = new_game_id;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  Message succ;
  succ.type = MSG_GAME_STATUS_CHANGE;
  succ.payload.status_change.game_id = new_game_id;
  succ.payload.status_change.new_state = GAME_WAITING;
  snprintf(succ.payload.status_change.message,
      sizeof(succ.payload.status_change.message),
      "Game created successfully – waiting for opponent");
  send_message(client_socket, &succ);

  broadcast_lobby_update();
}

void handle_join_request(int client_socket, uint32_t game_id) {
  printf("[Handler] Received JOIN_GAME for game_id=%u from socket %d\n", game_id, client_socket);
  if (client_is_playing(client_socket)) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Already in a game");
    send_message(client_socket, &err);
    return;
  }

  char username[32] = "Unknown";
  find_username_by_socket(client_socket, username, sizeof(username));

  int ret = game_manager_join_game(game_manager, game_id, client_socket,
      username);
  if (ret == -1) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Game not available for joining");
    send_message(client_socket, &err);
    return;
  }
  if (ret == 1) {
    // send request to owner
    int owner_sock = game_manager_get_owner_socket(game_manager, game_id);
    if (owner_sock != -1) {
      Message req;
      memset(&req, 0, sizeof(req));
      req.type = MSG_JOIN_REQUEST;
      req.payload.join_request.game_id = game_id;
      req.payload.join_request.requesting_player_id = client_socket;
      strncpy(req.payload.join_request.requesting_player_name, username,
          sizeof(req.payload.join_request.requesting_player_name)-1);
      req.payload.join_request.requesting_player_name[
        sizeof(req.payload.join_request.requesting_player_name)-1] = '\0';
      send_message(owner_sock, &req);
      printf("[Handler] Sent join request to owner (socket %d) for game %u from %s\n",
          owner_sock, game_id, username);
    }
  }
}

void handle_join_decision(int client_socket, bool accepted, uint32_t game_id) {
  Player assigned_role;
  int ret = game_manager_join_decision(game_manager, game_id, accepted ? 1 : 0, &assigned_role);
  if (ret < 0) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "No pending join request for this game");
    send_message(client_socket, &err);
    return;
  }

  if (accepted) {
    int x_sock, o_sock;
    if (game_manager_get_players(game_manager, game_id, &x_sock, &o_sock) < 0) {
      fprintf(stderr, "[Handler] Failed to get players after join decision\n");
      return;
    }

    // Send game start messages
    Message start_x, start_o;
    memset(&start_x, 0, sizeof(start_x));
    memset(&start_o, 0, sizeof(start_o));
    start_x.type = MSG_GAME_START;
    start_x.payload.game_start.game_id = game_id;
    start_x.payload.game_start.player_role = PLAYER_X;
    start_x.payload.game_start.first_player = PLAYER_X;

    start_o.type = MSG_GAME_START;
    start_o.payload.game_start.game_id = game_id;
    start_o.payload.game_start.player_role = PLAYER_O;
    start_o.payload.game_start.first_player = PLAYER_X;

    send_message(x_sock, &start_x);
    send_message(o_sock, &start_o);

    // Send initial board state to both
    GameCommonState state;
    if (game_manager_get_game_state(game_manager, game_id, &state) == 0) {
      client_set_playing(x_sock, true, game_id);
      client_set_playing(o_sock, true, game_id);
      Message state_x, state_o;
      memset(&state_x, 0, sizeof(state_x));
      memset(&state_o, 0, sizeof(state_o));
      state_x.type = MSG_GAME_STATE;
      state_x.payload.game_state.game_id = game_id;
      state_x.payload.game_state.state_data = state;
      state_x.payload.game_state.state_data.player_role = PLAYER_X;
      snprintf(state_x.payload.game_state.message,
          sizeof(state_x.payload.game_state.message),
          "Game started");

      state_o = state_x;
      state_o.payload.game_state.state_data.player_role = PLAYER_O;

      send_message(x_sock, &state_x);
      send_message(o_sock, &state_o);
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].is_active) {
        if (clients[i].socket == x_sock || clients[i].socket == o_sock) {
          clients[i].is_playing = true;
          clients[i].current_game_id = game_id;
        }
      }
    }
    pthread_mutex_unlock(&clients_mutex);
  } else {
    // Reject and notify waiting player
    int waiting_sock;
    if (game_manager_get_waiting_socket(game_manager, game_id, &waiting_sock) == 0 &&
        waiting_sock != -1) {
      Message err;
      err.type = MSG_ERROR;
      snprintf(err.payload.error.error_message,
          sizeof(err.payload.error.error_message),
          "Join request was rejected by game owner");
      send_message(waiting_sock, &err);
    }
  }

  broadcast_lobby_update();
}

void handle_move(int client_socket, Message* msg) {
  uint32_t game_id = msg->payload.move.game_id;
  uint8_t row = msg->payload.move.row;
  uint8_t col = msg->payload.move.col;
  bool finished;
  int winner;

  int ret = game_manager_move(game_manager, game_id, client_socket,
      row, col, &finished, &winner);
  if (ret < 0) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Invalid move or not your turn");
    send_message(client_socket, &err);
    return;
  }

  int x_sock, o_sock;
  if (game_manager_get_players(game_manager, game_id, &x_sock, &o_sock) < 0) {
    fprintf(stderr, "[Handler] Failed to get players after move\n");
    return;
  }

  // Get current game state
  GameCommonState state;
  if (game_manager_get_game_state(game_manager, game_id, &state) < 0) {
    fprintf(stderr, "[Handler] Failed to get game state after move\n");
    return;
  }

  if (finished) {
    Message over_msg;
    memset(&over_msg, 0, sizeof(over_msg));
    over_msg.type = MSG_GAME_OVER;
    over_msg.payload.game_over.game_id = game_id;
    over_msg.payload.game_over.winner = winner;
    snprintf(over_msg.payload.game_over.message,
        sizeof(over_msg.payload.game_over.message),
        winner == RESULT_DRAW ? "Game ended in a draw!" :
        (winner == PLAYER_X ? "Player X wins!" : "Player O wins!"));
    send_message(x_sock, &over_msg);
    send_message(o_sock, &over_msg);

    if (winner == RESULT_DRAW) {
      // draw
      Message post_msg;
      memset(&post_msg, 0, sizeof(post_msg));
      post_msg.type = MSG_POST_GAME_OPTIONS;
      post_msg.payload.post_game_options.game_id = game_id;
      send_message(x_sock, &post_msg);
      send_message(o_sock, &post_msg);
    } else {
      // win 
      int winner_sock = (winner == PLAYER_X) ? x_sock : o_sock;
      Message post_msg;
      memset(&post_msg, 0, sizeof(post_msg));
      post_msg.type = MSG_POST_GAME_OPTIONS;
      post_msg.payload.post_game_options.game_id = game_id;
      send_message(winner_sock, &post_msg);
    }
  } else {
    // Send updated state to both (with correct player_role)
    Message state_x, state_o;
    memset(&state_x, 0, sizeof(state_x));
    memset(&state_o, 0, sizeof(state_o));
    state_x.type = MSG_GAME_STATE;
    state_x.payload.game_state.game_id = game_id;
    state_x.payload.game_state.state_data = state;
    state_x.payload.game_state.state_data.player_role = PLAYER_X;
    snprintf(state_x.payload.game_state.message,
        sizeof(state_x.payload.game_state.message),
        "Board updated");

    state_o = state_x;
    state_o.payload.game_state.state_data.player_role = PLAYER_O;

    send_message(x_sock, &state_x);
    send_message(o_sock, &state_o);
  }
}

void handle_post_game_decision(int client_socket, uint32_t game_id, bool wants_to_continue) {
  printf("[Handler] Post-game decision: socket=%d, game=%u, wants=%d\n",
      client_socket, game_id, wants_to_continue);

  bool both_decided;
  int ret = game_manager_post_game_decision(game_manager, game_id,
      client_socket,
      wants_to_continue ? 1 : 0,
      &both_decided);
  if (ret < 0) {
    Message err;
    err.type = MSG_ERROR;
    snprintf(err.payload.error.error_message,
        sizeof(err.payload.error.error_message),
        "Post‑game decision failed");
    send_message(client_socket, &err);
    return;
  }

  if (!wants_to_continue) {
    client_set_playing(client_socket, false, 0);
    game_manager_leave_game(game_manager, game_id, client_socket);
    printf("[Handler] Player %d left game %u (chose not to continue)\n", client_socket, game_id);
  }

  GameCommonState state;
  int state_ret = game_manager_get_game_state(game_manager, game_id, &state);

  if (both_decided) {
    if (state_ret < 0) {
      printf("[Handler] Game %u no longer exists after post-game decision\n", game_id);
      broadcast_lobby_update();
      return;
    }

    if (state.state == GAME_WAITING || state.state == GAME_IN_PROGRESS) {
      int x_sock, o_sock;
      if (game_manager_get_players(game_manager, game_id, &x_sock, &o_sock) == 0) {
        if (!wants_to_continue) {
          if (x_sock == client_socket) x_sock = -1;
          if (o_sock == client_socket) o_sock = -1;
        }
        if (o_sock == -1) {
          if (x_sock != -1) {
            client_set_playing(x_sock, true, game_id);
            Message status;
            status.type = MSG_GAME_STATUS_CHANGE;
            status.payload.status_change.game_id = game_id;
            status.payload.status_change.new_state = GAME_WAITING;
            snprintf(status.payload.status_change.message,
                sizeof(status.payload.status_change.message),
                "You are now owner – waiting for opponent");
            send_message(x_sock, &status);
          }
        } else {
          // 2 players stay case
          client_set_playing(x_sock, true, game_id);
          client_set_playing(o_sock, true, game_id);
          
          Message start_x, start_o;
          memset(&start_x, 0, sizeof(start_x));
          memset(&start_o, 0, sizeof(start_o));
          start_x.type = MSG_GAME_START;
          start_x.payload.game_start.game_id = game_id;
          start_x.payload.game_start.player_role = PLAYER_X;
          start_x.payload.game_start.first_player = PLAYER_X;

          start_o.type = MSG_GAME_START;
          start_o.payload.game_start.game_id = game_id;
          start_o.payload.game_start.player_role = PLAYER_O;
          start_o.payload.game_start.first_player = PLAYER_X;

          send_message(x_sock, &start_x);
          send_message(o_sock, &start_o);

          GameCommonState new_state;
          if (game_manager_get_game_state(game_manager, game_id, &new_state) == 0) {
            Message state_x, state_o;
            memset(&state_x, 0, sizeof(state_x));
            memset(&state_o, 0, sizeof(state_o));
            state_x.type = MSG_GAME_STATE;
            state_x.payload.game_state.game_id = game_id;
            state_x.payload.game_state.state_data = new_state;
            state_x.payload.game_state.state_data.player_role = PLAYER_X;
            snprintf(state_x.payload.game_state.message,
                sizeof(state_x.payload.game_state.message),
                "Game restarted");

            state_o = state_x;
            state_o.payload.game_state.state_data.player_role = PLAYER_O;

            send_message(x_sock, &state_x);
            send_message(o_sock, &state_o);
          }
        }
      }
    } else {
      printf("[Handler] Game %u is finished, not re-enabling players\n", game_id);
    }
    broadcast_lobby_update();
  }
}

void handle_leave_game(int client_socket, uint32_t game_id) {
  printf("[Handler] Received leave game request for game %u from socket %d\n", game_id, client_socket);
  int ret = game_manager_leave_game(game_manager, game_id, client_socket);
  if (ret < 0) {
    fprintf(stderr, "[Handler] Failed to leave game %u for socket %d\n", game_id, client_socket);
    // Even if leave fails, the client should not be considered in a game anymore
    client_set_playing(client_socket, false, 0);
  } else {
    printf("[Handler] Player left game %u\n", game_id);
    client_set_playing(client_socket, false, 0);
  }
  broadcast_lobby_update();
}
