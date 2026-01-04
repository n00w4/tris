#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum MessageType {
  MSG_CREATE_GAME = 1,
  MSG_JOIN_GAME,
  MSG_MOVE,
  MSG_LEAVE_GAME,
  MSG_LIST_GAMES,
  MSG_GAME_STATE,
  MSG_GAME_LIST,
  MSG_ERROR
} MessageType;

/**
 * Payload for MSG_MOVE
 */
typedef struct MovePayload {
  uint8_t row;
  uint8_t col;
} MovePayload;

/**
 * Payload for MSG_GAME_STATE
 */
typedef struct GameStatePayload {
  char board[3][3];
  uint8_t current_turn;
  uint8_t state;
  char message[64];
} GameStatePayload;

/**
 * Payload for MSG_ERROR
 */
typedef struct ErrorPayload {
  char error_message[64];
} ErrorPayload;

/**
 * Payload for MSG_GAME_LIST
 */
typedef struct GameListPayload {
  uint32_t game_ids[10];
  uint8_t game_count;
  uint8_t game_states[10];
} GameListPayload;

typedef struct Message {
  uint8_t type;
  uint32_t game_id;

  union {
    MovePayload move;
    GameStatePayload game_state;
    ErrorPayload error;
    GameListPayload game_list;
  } payload;
} Message;

/**
 * Sends a message
 */
int send_message(int socket, const Message* msg);

/**
 * Receives a message
 */
int receive_message(int socket, Message* msg);

#endif

