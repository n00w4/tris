#include "../include/protocol.h"

#include <stddef.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t type;
  uint32_t length;
} __attribute__((packed)) MessageHeader;

static void write_uint32(uint8_t **buf, uint32_t val) {
  uint32_t net = htonl(val);
  memcpy(*buf, &net, sizeof(uint32_t));
  *buf += sizeof(uint32_t);
}

static uint32_t read_uint32(const uint8_t **buf) {
  uint32_t net;
  memcpy(&net, *buf, sizeof(uint32_t));
  *buf += sizeof(uint32_t);
  return ntohl(net);
}

static void write_uint8(uint8_t **buf, uint8_t val) {
  **buf = val;
  (*buf)++;
}

static uint8_t read_uint8(const uint8_t **buf) {
  uint8_t val = **buf;
  (*buf)++;
  return val;
}

static int serialize_move(const MovePayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1 + 1;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, src->row);
  write_uint8(&p, src->col);
  *out_len = needed;
  return 0;
}

static int deserialize_move(const uint8_t *src, size_t src_len, MovePayload *dst) {
  if (src_len != 4 + 1 + 1) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->row = read_uint8(&p);
  dst->col = read_uint8(&p);
  return 0;
}

static int serialize_game_state(const GameStatePayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 9 + 5 + 64;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      write_uint8(&p, (uint8_t)src->state_data.board[i][j]);
    }
  }
  write_uint8(&p, (uint8_t)src->state_data.current_turn);
  write_uint8(&p, (uint8_t)src->state_data.state);
  write_uint8(&p, (uint8_t)src->state_data.result_player_x);
  write_uint8(&p, (uint8_t)src->state_data.result_player_o);
  write_uint8(&p, (uint8_t)src->state_data.player_role);
  memcpy(p, src->message, 64);
  p += 64;

  *out_len = needed;
  return 0;
}

static int deserialize_game_state(const uint8_t *src, size_t src_len, GameStatePayload *dst) {
  if (src_len != 4 + 9 + 5 + 64) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      dst->state_data.board[i][j] = (char)read_uint8(&p);
    }
  }
  dst->state_data.current_turn = (Player)read_uint8(&p);
  dst->state_data.state = (GameState)read_uint8(&p);
  dst->state_data.result_player_x = (GameResult)read_uint8(&p);
  dst->state_data.result_player_o = (GameResult)read_uint8(&p);
  dst->state_data.player_role = (Player)read_uint8(&p);
  memcpy(dst->message, p, 64);
  return 0;
}

static int serialize_error(const ErrorPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 64;
  if (dst_cap < needed) { return -1; }
  memcpy(dst, src->error_message, 64);
  *out_len = needed;
  return 0;
}

static int deserialize_error(const uint8_t *src, size_t src_len, ErrorPayload *dst) {
  if (src_len != 64) { return -1; }
  memcpy(dst->error_message, src, 64);
  return 0;
}

static int serialize_game_list(const GameListPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 40 + 1 + 10;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  for (int i = 0; i < 10; i++) { write_uint32(&p, src->game_ids[i]); }
  write_uint8(&p, src->game_count);
  for (int i = 0; i < 10; i++) { write_uint8(&p, (uint8_t)src->game_states[i]); }
  *out_len = needed;
  return 0;
}

static int deserialize_game_list(const uint8_t *src, size_t src_len, GameListPayload *dst) {
  if (src_len != 40 + 1 + 10) { return -1; }
  const uint8_t *p = src;
  for (int i = 0; i < 10; i++) { dst->game_ids[i] = read_uint32(&p); }
  dst->game_count = read_uint8(&p);
  for (int i = 0; i < 10; i++) { dst->game_states[i] = (GameState)read_uint8(&p); }
  return 0;
}

static int serialize_game_start(const GameStartPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1 + 1;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, (uint8_t)src->player_role);
  write_uint8(&p, (uint8_t)src->first_player);
  *out_len = needed;
  return 0;
}

static int deserialize_game_start(const uint8_t *src, size_t src_len, GameStartPayload *dst) {
  if (src_len != 4 + 1 + 1) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->player_role = (Player)read_uint8(&p);
  dst->first_player = (Player)read_uint8(&p);
  return 0;
}

static int serialize_game_over(const GameOverPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1 + 64;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, src->winner);
  memcpy(p, src->message, 64);
  p += 64;
  *out_len = needed;
  return 0;
}

static int deserialize_game_over(const uint8_t *src, size_t src_len, GameOverPayload *dst) {
  if (src_len != 4 + 1 + 64) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->winner = read_uint8(&p);
  memcpy(dst->message, p, 64);
  return 0;
}

static int serialize_join_request(const JoinRequestPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 4 + 32;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint32(&p, src->requesting_player_id);
  memcpy(p, src->requesting_player_name, 32);
  p += 32;
  *out_len = needed;
  return 0;
}

static int deserialize_join_request(const uint8_t *src, size_t src_len, JoinRequestPayload *dst) {
  if (src_len != 4 + 4 + 32) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->requesting_player_id = read_uint32(&p);
  memcpy(dst->requesting_player_name, p, 32);
  return 0;
}

static int serialize_join_decision(const JoinDecisionPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, src->accepted ? 1 : 0);
  *out_len = needed;
  return 0;
}

static int deserialize_join_decision(const uint8_t *src, size_t src_len, JoinDecisionPayload *dst) {
  if (src_len != 4 + 1) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->accepted = (read_uint8(&p) != 0);
  return 0;
}

static int serialize_lobby_update(const LobbyUpdatePayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 1 + 10 * 41;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint8(&p, src->game_count);
  for (int i = 0; i < 10; i++) {
    write_uint32(&p, src->games[i].game_id);
    write_uint8(&p, (uint8_t)src->games[i].state);
    memcpy(p, src->games[i].owner_name, 32);
    p += 32;
    write_uint32(&p, (uint32_t)src->games[i].players_connected);
  }
  *out_len = needed;
  return 0;
}

static int deserialize_lobby_update(const uint8_t *src, size_t src_len, LobbyUpdatePayload *dst) {
  if (src_len != 1 + 10 * 41) { return -1; }
  const uint8_t *p = src;
  dst->game_count = read_uint8(&p);
  for (int i = 0; i < 10; i++) {
    dst->games[i].game_id = read_uint32(&p);
    dst->games[i].state = (GameState)read_uint8(&p);
    memcpy(dst->games[i].owner_name, p, 32);
    p += 32;
    dst->games[i].players_connected = (int)read_uint32(&p);
  }
  return 0;
}

static int serialize_status_change(const GameStatusChangePayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1 + 128;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, (uint8_t)src->new_state);
  memcpy(p, src->message, 128);
  p += 128;
  *out_len = needed;
  return 0;
}

static int deserialize_status_change(const uint8_t *src, size_t src_len, GameStatusChangePayload *dst) {
  if (src_len != 4 + 1 + 128) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->new_state = (GameState)read_uint8(&p);
  memcpy(dst->message, p, 128);
  return 0;
}

static int serialize_post_game_options(const PostGameOptionsPayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 4 + 1;
  if (dst_cap < needed) { return -1; }

  uint8_t *p = dst;
  write_uint32(&p, src->game_id);
  write_uint8(&p, src->winner_wants_to_continue ? 1 : 0);
  *out_len = needed;
  return 0;
}

static int deserialize_post_game_options(const uint8_t *src, size_t src_len, PostGameOptionsPayload *dst) {
  if (src_len != 4 + 1) { return -1; }
  const uint8_t *p = src;
  dst->game_id = read_uint32(&p);
  dst->winner_wants_to_continue = (read_uint8(&p) != 0);
  return 0;
}

static int serialize_set_username(const SetUsernamePayload *src, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  const size_t needed = 32;
  if (dst_cap < needed) { return -1; }
  memcpy(dst, src->username, 32);
  *out_len = needed;
  return 0;
}

static int deserialize_set_username(const uint8_t *src, size_t src_len, SetUsernamePayload *dst) {
  if (src_len != 32) { return -1; }
  memcpy(dst->username, src, 32);
  return 0;
}

int send_message(int socket, const Message *msg) {
  if (!msg) { return -1; }

  uint8_t payload_buf[1024];
  memset(payload_buf, 0, sizeof(payload_buf));

  size_t payload_len = 0;
  int ret = -1;

  switch (msg->type) {
    case MSG_MOVE:
      ret = serialize_move(&msg->payload.move, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_GAME_STATE:
      ret = serialize_game_state(&msg->payload.game_state, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_ERROR:
      ret = serialize_error(&msg->payload.error, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_GAME_LIST:
      ret = serialize_game_list(&msg->payload.game_list, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_GAME_START:
      ret = serialize_game_start(&msg->payload.game_start, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_GAME_OVER:
      ret = serialize_game_over(&msg->payload.game_over, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_JOIN_REQUEST:
      ret = serialize_join_request(&msg->payload.join_request, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_JOIN_DECISION:
      ret = serialize_join_decision(&msg->payload.join_decision, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_LOBBY_UPDATE:
      ret = serialize_lobby_update(&msg->payload.lobby_update, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_GAME_STATUS_CHANGE:
      ret = serialize_status_change(&msg->payload.status_change, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_POST_GAME_OPTIONS:
      ret = serialize_post_game_options(&msg->payload.post_game_options, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_SET_USERNAME:
      ret = serialize_set_username(&msg->payload.set_username, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_JOIN_GAME:
      ret = serialize_join_request(&msg->payload.join_request, payload_buf, sizeof(payload_buf), &payload_len);
      break;
    case MSG_CREATE_GAME:
    case MSG_LEAVE_GAME:
    case MSG_LIST_GAMES:
      payload_len = 0;
      ret = 0;
      break;
    default:
      fprintf(stderr, "[protocol] send_message: unknown message type %d\n", msg->type);
      return -1;
  }

  if (ret < 0) {
    fprintf(stderr, "[protocol] send_message: serialization failed for type %d\n", msg->type);
    return -1;
  }

  MessageHeader header;
  memset(&header, 0, sizeof(header));
  header.magic = htonl(PROTOCOL_MAGIC);
  header.version = htonl(PROTOCOL_VERSION);
  header.type = htonl((uint32_t)msg->type);
  header.length = htonl((uint32_t)payload_len);

  size_t total_sent = 0;
  const uint8_t *hdr_ptr = (const uint8_t*)&header;
  size_t hdr_size = sizeof(header);
  while (total_sent < hdr_size) {
    ssize_t n = send(socket, hdr_ptr + total_sent, hdr_size - total_sent, 0);
    if (n <= 0) { perror("[protocol] send header"); return -1; }
    total_sent += (size_t)n;
  }

  if (payload_len > 0) {
    total_sent = 0;
    while (total_sent < payload_len) {
      ssize_t n = send(socket, payload_buf + total_sent, payload_len - total_sent, 0);
      if (n <= 0) { perror("[protocol] send payload"); return -1; }
      total_sent += (size_t)n;
    }
  }

  return 0;
}

int receive_message(int socket, Message *msg) {
  if (!msg) { return -1; }

  MessageHeader header;
  size_t total_read = 0;
  uint8_t *hdr_ptr = (uint8_t*)&header;
  size_t hdr_size = sizeof(header);
  while (total_read < hdr_size) {
    ssize_t n = recv(socket, hdr_ptr + total_read, hdr_size - total_read, 0);
    if (n <= 0) {
      if (n == 0) { fprintf(stderr, "[protocol] receive_message: connection closed\n"); }
      else { perror("[protocol] recv header"); }
      return -1;
    }
    total_read += (size_t)n;
  }

  header.magic = ntohl(header.magic);
  header.version = ntohl(header.version);
  header.type = ntohl(header.type);
  header.length = ntohl(header.length);

  if (header.magic != PROTOCOL_MAGIC) {
    fprintf(stderr, "[protocol] receive_message: invalid magic (got 0x%08x, expected 0x%08x)\n", header.magic, PROTOCOL_MAGIC);
    return -1;
  }
  if (header.version != PROTOCOL_VERSION) {
    fprintf(stderr, "[protocol] receive_message: unsupported version %u\n", header.version);
    return -1;
  }
  if (header.length > 1024) {
    fprintf(stderr, "[protocol] receive_message: payload too large (%u)\n", header.length);
    return -1;
  }

  uint8_t payload_buf[1024];
  size_t payload_len = header.length;
  total_read = 0;
  while (total_read < payload_len) {
    ssize_t n = recv(socket, payload_buf + total_read, payload_len - total_read, 0);
    if (n <= 0) {
      if (n == 0) { fprintf(stderr, "[protocol] receive_message: connection closed during payload read\n"); }
      else { perror("[protocol] recv payload"); }
      return -1;
    }
    total_read += (size_t)n;
  }

  msg->type = (MessageType)header.type;
  int ret = -1;
  switch (msg->type) {
    case MSG_MOVE:
      ret = deserialize_move(payload_buf, payload_len, &msg->payload.move);
      break;
    case MSG_GAME_STATE:
      ret = deserialize_game_state(payload_buf, payload_len, &msg->payload.game_state);
      break;
    case MSG_ERROR:
      ret = deserialize_error(payload_buf, payload_len, &msg->payload.error);
      break;
    case MSG_GAME_LIST:
      ret = deserialize_game_list(payload_buf, payload_len, &msg->payload.game_list);
      break;
    case MSG_GAME_START:
      ret = deserialize_game_start(payload_buf, payload_len, &msg->payload.game_start);
      break;
    case MSG_GAME_OVER:
      ret = deserialize_game_over(payload_buf, payload_len, &msg->payload.game_over);
      break;
    case MSG_JOIN_REQUEST:
      ret = deserialize_join_request(payload_buf, payload_len, &msg->payload.join_request);
      break;
    case MSG_JOIN_DECISION:
      ret = deserialize_join_decision(payload_buf, payload_len, &msg->payload.join_decision);
      break;
    case MSG_LOBBY_UPDATE:
      ret = deserialize_lobby_update(payload_buf, payload_len, &msg->payload.lobby_update);
      break;
    case MSG_GAME_STATUS_CHANGE:
      ret = deserialize_status_change(payload_buf, payload_len, &msg->payload.status_change);
      break;
    case MSG_POST_GAME_OPTIONS:
      ret = deserialize_post_game_options(payload_buf, payload_len, &msg->payload.post_game_options);
      break;
    case MSG_SET_USERNAME:
      ret = deserialize_set_username(payload_buf, payload_len, &msg->payload.set_username);
      break;
    case MSG_JOIN_GAME:
      ret = deserialize_join_request(payload_buf, payload_len, &msg->payload.join_request);
      break;
    case MSG_CREATE_GAME:
    case MSG_LEAVE_GAME:
    case MSG_LIST_GAMES:
      if (payload_len != 0) { fprintf(stderr, "[protocol] receive_message: type %d expects empty, got %zu\n", msg->type, payload_len); return -1; }
      ret = 0;
      break;
    default:
      fprintf(stderr, "[protocol] receive_message: unknown type %d\n", msg->type);
      return -1;
  }

  if (ret < 0) {
    fprintf(stderr, "[protocol] receive_message: deserialization failed for type %d\n", msg->type);
    return -1;
  }

  return 0;
}
