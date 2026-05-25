#include "ui/ui.h"
#include "ui/ui_events.h"
#include "utils/queue.h"
#include "protocol.h"
#include "utils/utils.h"
#include "network/network.h"

#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <stdatomic.h>
#include <stdio.h>
#include <ctype.h>

#define CELL_H 3
#define CELL_W 6
#define START_X 10
#define START_Y 5
#define MAX_MENU_ITEMS 10

typedef enum {
  SCREEN_MAIN_MENU,
  SCREEN_LOBBY,
  SCREEN_GAME_WAITING,
  SCREEN_IN_GAME,
  SCREEN_JOIN_REQUEST,
  SCREEN_POST_GAME,
  SCREEN_DISCONNECTED,
  SCREEN_SETTINGS,
  SCREEN_HELP,
  SCREEN_GAME_OVER
} ScreenState;

typedef enum {
  SETTINGS_NAV,
  SETTINGS_EDIT
} SettingsMode;

typedef struct {
  ScreenState screen;
  uint32_t current_game_id;
  Player player_role;
  GameCommonState game_state;
  LobbyGameInfo lobby_games[10];
  uint8_t lobby_count;
  char error_message[256];
  char join_requester_name[32];
  uint32_t join_game_id;
  char post_game_message[128];
  int cursor_row;
  int cursor_col;
  time_t disconnected_at;

  Config config;
  network_ctx* net_ctx;
  bool is_connected;
  int settings_active_field;
  SettingsMode settings_mode;
  char settings_edit_buf[32];
  int menu_selection;
  int last_game_winner;           // winner of the last game (PLAYER_X, PLAYER_O, RESULT_DRAW, RESULT_NONE)
} UIState;

static atomic_int ui_running = 0;

static Message* alloc_message(void) {
  Message* msg = malloc(sizeof(Message));
  if (!msg) {
    fprintf(stderr, "[ui] OOM: cannot allocate Message\n");
    return NULL;
  }
  memset(msg, 0, sizeof(Message));
  return msg;
}

static void draw_status_bar(UIState* state) {
  if (state->error_message[0] != '\0') {
    attron(A_BOLD);
    mvprintw(LINES - 1, 0, "ERR: %.60s", state->error_message);
    attroff(A_BOLD);
  }
}

void ui_stop(void) {
  atomic_store(&ui_running, 0);
}

static void init_ncurses(void) {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  timeout(100);
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
  }
}

static bool ensure_network_connected(UIState* state, queue_t* to_net, queue_t* from_net) {
  if (state->is_connected && state->net_ctx && network_is_connected(state->net_ctx)) {
    return true;
  }
  if (state->net_ctx) {
    network_stop(state->net_ctx);
    network_wait(state->net_ctx);
    network_destroy(state->net_ctx);
    state->net_ctx = NULL;
    state->is_connected = false;
  }
  state->net_ctx = network_start(state->config.username, state->config.ip, atoi(state->config.port),
      from_net, to_net);
  if (!state->net_ctx) {
    strncpy(state->error_message, "Failed to connect to server", sizeof(state->error_message) - 1);
    return false;
  }
  napms(100);
  if (!network_is_connected(state->net_ctx)) {
    network_stop(state->net_ctx);
    network_wait(state->net_ctx);
    network_destroy(state->net_ctx);
    state->net_ctx = NULL;
    strncpy(state->error_message, "Could not connect to server", sizeof(state->error_message) - 1);
    return false;
  }
  state->is_connected = true;
  return true;
}

// menu action helper
static void do_menu_action(UIState* state, int action, queue_t* to_net, queue_t* from_net) {
  switch (action) {
    case 0: // create game
      if (ensure_network_connected(state, to_net, from_net)) {
        Message* cmd = alloc_message();
        if (cmd) {
          cmd->type = MSG_CREATE_GAME;
          queue_push(to_net, cmd);
        }
      }
      break;
    case 1: // join game (enters lobby)
      if (ensure_network_connected(state, to_net, from_net)) {
        Message* cmd = alloc_message();
        if (cmd) {
          cmd->type = MSG_LIST_GAMES;
          queue_push(to_net, cmd);
          state->screen = SCREEN_LOBBY;
        }
      }
      break;
    case 2: // settings
      read_config(&state->config);
      state->settings_active_field = 0;
      state->settings_mode = SETTINGS_NAV;
      state->screen = SCREEN_SETTINGS;
      break;
    case 3: // help
      state->screen = SCREEN_HELP;
      break;
    case 4: // quit
      ui_stop();
      break;
    default:
      break;
  }
}

// drawing functions
static void draw_main_menu(UIState* state) {
  clear();
  int mid_y = LINES / 2 - 4;
  int mid_x = (COLS - 20) / 2;
  mvprintw(mid_y, mid_x, "=== TRIS ===");

  const char* items[] = {
    "Create game",
    "Join game",
    "Settings",
    "Help",
    "Quit"
  };
  int item_count = 5;

  for (int i = 0; i < item_count; i++) {
    int y = mid_y + 2 + i;
    if (i == state->menu_selection) {
      attron(A_REVERSE);
      mvprintw(y, mid_x, "%s", items[i]);
      attroff(A_REVERSE);
    } else {
      mvprintw(y, mid_x, "%s", items[i]);
    }
  }

  mvprintw(mid_y + 2 + item_count + 1, (COLS - 40) / 2, "Use arrows to move, ENTER to select");
  draw_status_bar(state);
  refresh();
}

static void draw_lobby(UIState* state) {
  clear();
  mvprintw(1, (COLS - 30) / 2, "=== Available Games ===");
  int y = 3;
  for (int i = 0; i < state->lobby_count && i < 10; i++) {
    mvprintw(y++, 4, "%d - ID: %u - Owner: %s - (%d players) - STATUS: %s", i + 1,
      state->lobby_games[i].game_id,
      state->lobby_games[i].owner_name,
      state->lobby_games[i].players_connected,
      state->lobby_games[i].state == GAME_WAITING ? "Waiting" : "In-Game");
  }
  if (state->lobby_count == 0) {
    mvprintw(y, 4, "No games available.");
  }
  mvprintw(LINES - 2, 2, "Press number to join, 'r' to refresh, ESC to go back");
  draw_status_bar(state);
  refresh();
}

static void draw_game_waiting(UIState* state) {
  clear();
  mvprintw(LINES / 2 - 2, (COLS - 20) / 2, "Waiting for opponent...");
  mvprintw(LINES / 2, (COLS - 20) / 2, "Game ID: %u", state->current_game_id);
  mvprintw(LINES / 2 + 2, (COLS - 20) / 2, "Press ESC to cancel");
  draw_status_bar(state);
  refresh();
}

static void draw_game_board(UIState* state) {
  clear();
  int start_y = (LINES - 8) / 2;  // 8 rows (3 cells + 2 lines + space info)
  int start_x = (COLS - 19) / 2;  // 19 cols (3 cells * 5 + 2 lines * 2)

  // vertical inner lines (two)
  for (int j = 1; j < 3; j++) {
    int x = start_x + j * 6;
    mvvline(start_y, x, ACS_VLINE, 9); // total height 9 (3 cells * 3)
  }

  // horizontal inner lines (two)
  for (int i = 1; i < 3; i++) {
    int y = start_y + i * 3;
    mvhline(y, start_x, ACS_HLINE, 19);
    mvaddch(y, start_x + 6, ACS_PLUS);
    mvaddch(y, start_x + 12, ACS_PLUS);
  }

  // symbols in cells
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int y = start_y + i * 3 + 1; // vertical center
      int x = start_x + j * 6 + 2; // horizontal center (2 spaces from left border)
      char cell = state->game_state.board[i][j];
      if (i == state->cursor_row && j == state->cursor_col) {
        attron(A_REVERSE);
        mvprintw(y, x, " %c ", cell);
        attroff(A_REVERSE);
      } else {
        mvprintw(y, x, " %c ", cell);
      }
    }
  }

  // game info
  mvprintw(start_y + 10, start_x, "Turn: %s", state->game_state.current_turn == PLAYER_X ? "X" : "O");
  mvprintw(start_y + 11, start_x, "You are: %s", state->player_role == PLAYER_X ? "X" : "O");
  mvprintw(LINES - 3, 2, "Use arrow keys to move, ENTER to place, ESC to quit");
  draw_status_bar(state);
  refresh();
}

static void draw_join_request(UIState* state) {
  clear();
  mvprintw(LINES / 2 - 2, (COLS - 30) / 2, "Join request from: %s", state->join_requester_name);
  mvprintw(LINES / 2, (COLS - 30) / 2, "Accept? (y/n)");
  draw_status_bar(state);
  refresh();
}

static void draw_result_message(UIState* state, bool play_again) {
  char display_msg[256];
  if (state->last_game_winner == RESULT_DRAW) {
      snprintf(display_msg, sizeof(display_msg), "DRAW! %s", state->post_game_message);
  } else {
      int player_won = ((int)state->player_role == state->last_game_winner);
      snprintf(display_msg, sizeof(display_msg), "%s! %s",
        player_won ? "YOU WIN" : "YOU LOSE", state->post_game_message);
    }
  int msg_len = (int)strlen(display_msg);
  mvprintw(LINES / 2 - 2, (COLS - msg_len) / 2, "%s", display_msg);
  if (play_again) {
      mvprintw(LINES / 2, (COLS - 30) / 2, "Play again? (y/n)");
  } else {
      mvprintw(LINES / 2, (COLS - 30) / 2, "Press any key to return to menu...");
  }
  draw_status_bar(state);
  refresh();
}

static void draw_post_game(UIState* state) {
  clear();
  draw_result_message(state, true);
}

static void draw_game_over(UIState* state) {
  clear();
  draw_result_message(state, false);
}

static void draw_disconnected(UIState* state) {
  clear();
  mvprintw(LINES / 2, (COLS - 20) / 2, "Disconnected from server.");
  mvprintw(LINES / 2 + 1, (COLS - 20) / 2, "Exiting...");
  draw_status_bar(state);
  refresh();
}

static void draw_settings(UIState* state) {
  clear();
  attron(A_BOLD);
  mvprintw(2, (COLS - 16) / 2, "=== SETTINGS ===");
  attroff(A_BOLD);

  int y = 5;
  int x = (COLS - 30) / 2;

  if (state->settings_active_field == 0 && state->settings_mode == SETTINGS_NAV) {
    attron(A_REVERSE);
  }
  mvprintw(y, x, "Username: ");
  if (state->settings_active_field == 0 && state->settings_mode == SETTINGS_EDIT) {
    printw("%-20s", state->settings_edit_buf);
  } else {
    printw("%s", state->config.username);
  }
  if (state->settings_active_field == 0 && state->settings_mode == SETTINGS_NAV) {
    attroff(A_REVERSE);
  }

  y += 2;
  if (state->settings_active_field == 1 && state->settings_mode == SETTINGS_NAV) {
    attron(A_REVERSE);
  }
  mvprintw(y, x, "IP: ");
  if (state->settings_active_field == 1 && state->settings_mode == SETTINGS_EDIT) {
    printw("%-15s", state->settings_edit_buf);
  } else {
    printw("%s", state->config.ip);
  }
  if (state->settings_active_field == 1 && state->settings_mode == SETTINGS_NAV) {
    attroff(A_REVERSE);
  }

  y += 2;
  if (state->settings_active_field == 2 && state->settings_mode == SETTINGS_NAV) {
    attron(A_REVERSE);
  }
  mvprintw(y, x, "Port: ");
  if (state->settings_active_field == 2 && state->settings_mode == SETTINGS_EDIT) {
    printw("%-5s", state->settings_edit_buf);
  } else {
    printw("%s", state->config.port);
  }
  if (state->settings_active_field == 2 && state->settings_mode == SETTINGS_NAV) {
    attroff(A_REVERSE);
  }

  y += 4;
  if (state->settings_active_field == 3 && state->settings_mode == SETTINGS_NAV) {
    attron(A_REVERSE | A_BOLD);
  }
  mvprintw(y, x + 10, "[ SAVE ]");
  if (state->settings_active_field == 3 && state->settings_mode == SETTINGS_NAV) {
    attroff(A_REVERSE | A_BOLD);
  }

  if (state->settings_mode == SETTINGS_EDIT) {
    mvprintw(LINES - 2, 2, "Editing: type and press ENTER to confirm, ESC to cancel");
  } else {
    mvprintw(LINES - 2, 2, "Arrows: navigate   ENTER: edit/save   ESC: exit");
  }
  draw_status_bar(state);
  refresh();
}

static void draw_help(UIState* state) {
  clear();
  attron(A_BOLD);
  mvprintw(1, COLS / 2 - 10, "=== TRIS HELP ===");
  attroff(A_BOLD);

  int y = 4;
  mvprintw(y++, 2, "Goal of the game:");
  mvprintw(y++, 4, "- Align three identical symbols in a row.");
  y++;
  mvprintw(y++, 2, "Multiplayer Features:");
  mvprintw(y++, 4, "- Create Game: Create a new game and wait for opponents");
  mvprintw(y++, 4, "- Join Game: Browse and join available games");
  mvprintw(y++, 4, "- Lobby System: See all available games in real-time");
  mvprintw(y++, 4, "- Join Requests: Accept or reject players wanting to join");
  mvprintw(y++, 4, "- Post-Game Options: Winners can choose to stay as owner");
  y++;
  mvprintw(y++, 2, "How to play:");
  mvprintw(y++, 4, "- Use ARROW KEYS to navigate menus and move cursor on board.");
  mvprintw(y++, 4, "- Press ENTER to select an option or place a move.");
  mvprintw(y++, 4, "- Press ESC to go back (except main menu where ESC exits).");
  mvprintw(y++, 4, "- During game, you can also LEFT CLICK to select a cell.");
  y++;
  mvprintw(y++, 2, "Press any key to return to main menu...");
  draw_status_bar(state);
  refresh();
}

// input handling
static void handle_keyboard(UIState* state, int ch, queue_t* to_net, queue_t* from_net) {
  if (state->error_message[0] != '\0') {
    if (ch == '\n' || ch == ' ' || ch == 27) {
      memset(state->error_message, 0, sizeof(state->error_message));
      state->screen = SCREEN_LOBBY;
    }
    return;
  }

  switch (state->screen) {
    case SCREEN_MAIN_MENU: {
                             switch (ch) {
                               case KEY_UP:
                                 if (state->menu_selection > 0) {
                                   state->menu_selection--;
                                 }
                                 break;
                               case KEY_DOWN:
                                 if (state->menu_selection < 4) {
                                   state->menu_selection++;
                                 }
                                 break;
                               case '\n':
                                 do_menu_action(state, state->menu_selection, to_net, from_net);
                                 break;
                               case 27:  // ESC
                                 ui_stop();
                                 break;
                               default:
                                 // fallback to numeric keys (1-5)
                                 if (ch >= '1' && ch <= '5') {
                                   int num = ch - '0';
                                   do_menu_action(state, num - 1, to_net, from_net);
                                 }
                                 break;
                             }
                             break;
                           }

    case SCREEN_LOBBY: {
                         if (ch >= '1' && ch <= '9') {
                           int idx = ch - '1';
                           if (idx < state->lobby_count) {
                             Message* cmd = alloc_message();
                             if (cmd) {
                               cmd->type = MSG_JOIN_GAME;
                               cmd->payload.join_request.game_id = state->lobby_games[idx].game_id;
                               state->current_game_id = state->lobby_games[idx].game_id;
                               queue_push(to_net, cmd);
                               state->screen = SCREEN_GAME_WAITING;
                             }
                           }
                         } else if (ch == 'r' || ch == 'R') {
                           Message* cmd = alloc_message();
                           if (cmd) {
                             cmd->type = MSG_LIST_GAMES;
                             queue_push(to_net, cmd);
                           }
                         } else if (ch == 27) {
                           state->screen = SCREEN_MAIN_MENU;
                         }
                         break;
                       }

    case SCREEN_GAME_WAITING: {
                                if (ch == 27) {
                                  Message* cmd = alloc_message();
                                  if (cmd) {
                                    cmd->type = MSG_LEAVE_GAME;
                                    cmd->payload.move.game_id = state->current_game_id;
                                    queue_push(to_net, cmd);
                                  }
                                  state->screen = SCREEN_MAIN_MENU;
                                }
                                break;
                              }

    case SCREEN_IN_GAME: {
                           switch (ch) {
                             case KEY_UP:
                               state->cursor_row = (state->cursor_row - 1 + 3) % 3;
                               break;
                             case KEY_DOWN:
                               state->cursor_row = (state->cursor_row + 1) % 3;
                               break;
                             case KEY_LEFT:
                               state->cursor_col = (state->cursor_col - 1 + 3) % 3;
                               break;
                             case KEY_RIGHT:
                               state->cursor_col = (state->cursor_col + 1) % 3;
                               break;
                             case '\n':
                             case ' ':
                               if (state->game_state.board[state->cursor_row][state->cursor_col] == ' ' &&
                                   state->game_state.current_turn == state->player_role) {
                                 Message* cmd = alloc_message();
                                 if (cmd) {
                                   cmd->type = MSG_MOVE;
                                   cmd->payload.move.game_id = state->current_game_id;
                                   cmd->payload.move.row = (uint8_t)state->cursor_row;
                                   cmd->payload.move.col = (uint8_t)state->cursor_col;
                                   queue_push(to_net, cmd);
                                 }
                               }
                               break;
                             case 27: {
                                        Message* cmd = alloc_message();
                                        if (cmd) {
                                          cmd->type = MSG_LEAVE_GAME;
                                          cmd->payload.move.game_id = state->current_game_id;
                                          queue_push(to_net, cmd);
                                        }
                                        state->screen = SCREEN_MAIN_MENU;
                                        break;
                                      }
                             case KEY_MOUSE: {
                                               MEVENT event;
                                               if (getmouse(&event) == OK && (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED))) {
                                                 int start_y = (LINES - 8) / 2;
                                                 int start_x = (COLS - 19) / 2;
                                                 int r = -1, c = -1;

                                                 // clicked row
                                                 for (int i = 0; i < 3; i++) {
                                                   if (event.y >= start_y + i * 3 && event.y <= start_y + i * 3 + 2) {
                                                     r = i;
                                                     break;
                                                   }
                                                 }
                                                 // clicked col
                                                 for (int j = 0; j < 3; j++) {
                                                   if (event.x >= start_x + j * 6 && event.x <= start_x + j * 6 + 5) {
                                                     c = j;
                                                     break;
                                                   }
                                                 }

                                                 if (r != -1 && c != -1) {
                                                   state->cursor_row = r;
                                                   state->cursor_col = c;
                                                   if (state->game_state.board[r][c] == ' ' &&
                                                       state->game_state.current_turn == state->player_role) {
                                                     Message* cmd = alloc_message();
                                                     if (cmd) {
                                                       cmd->type = MSG_MOVE;
                                                       cmd->payload.move.game_id = state->current_game_id;
                                                       cmd->payload.move.row = (uint8_t)r;
                                                       cmd->payload.move.col = (uint8_t)c;
                                                       queue_push(to_net, cmd);
                                                     }
                                                   }
                                                 }
                                               }
                                               break;
                                             }
                           }
                           break;
                         }

    case SCREEN_JOIN_REQUEST: {
                                if (ch == 'y' || ch == 'Y') {
                                  Message* cmd = alloc_message();
                                  if (cmd) {
                                    cmd->type = MSG_JOIN_DECISION;
                                    cmd->payload.join_decision.game_id = state->join_game_id;
                                    cmd->payload.join_decision.accepted = 1;
                                    queue_push(to_net, cmd);
                                    state->screen = SCREEN_GAME_WAITING;
                                  }
                                } else if (ch == 'n' || ch == 'N') {
                                  Message* cmd = alloc_message();
                                  if (cmd) {
                                    cmd->type = MSG_JOIN_DECISION;
                                    cmd->payload.join_decision.game_id = state->join_game_id;
                                    cmd->payload.join_decision.accepted = 0;
                                    queue_push(to_net, cmd);
                                  }
                                  state->screen = SCREEN_GAME_WAITING;
                                } else if (ch == 27) {
                                  Message* cmd_decision = alloc_message();
                                  if (cmd_decision) {
                                    cmd_decision->type = MSG_JOIN_DECISION;
                                    cmd_decision->payload.join_decision.game_id = state->join_game_id;
                                    cmd_decision->payload.join_decision.accepted = 0;
                                    queue_push(to_net, cmd_decision);
                                  }
                                  Message* cmd_leave = alloc_message();
                                  if (cmd_leave) {
                                    cmd_leave->type = MSG_LEAVE_GAME;
                                    cmd_leave->payload.move.game_id = state->current_game_id;
                                    queue_push(to_net, cmd_leave);
                                  }
                                  state->current_game_id = 0;
                                  state->screen = SCREEN_MAIN_MENU;
                                }
                                break;
                              }

    case SCREEN_POST_GAME: {
                             if (ch == 'y' || ch == 'Y') {
                               Message* cmd = alloc_message();
                               if (cmd) {
                                 cmd->type = MSG_POST_GAME_OPTIONS;
                                 cmd->payload.post_game_options.game_id = state->current_game_id;
                                 cmd->payload.post_game_options.winner_wants_to_continue = 1;
                                 queue_push(to_net, cmd);
                                 state->screen = SCREEN_GAME_WAITING;
                               }
                             } else if (ch == 'n' || ch == 'N' || ch == 27) {
                               Message* cmd = alloc_message();
                               if (cmd) {
                                 cmd->type = MSG_POST_GAME_OPTIONS;
                                 cmd->payload.post_game_options.game_id = state->current_game_id;
                                 cmd->payload.post_game_options.winner_wants_to_continue = 0;
                                 queue_push(to_net, cmd);
                               }
                               state->screen = SCREEN_MAIN_MENU;
                             }
                             break;
                           }

    case SCREEN_GAME_OVER: {
                             // any key: send leave and return to main menu
                             Message* cmd = alloc_message();
                             if (cmd) {
                               cmd->type = MSG_LEAVE_GAME;
                               cmd->payload.move.game_id = state->current_game_id;
                               queue_push(to_net, cmd);
                               printf("[ui] Sending MSG_LEAVE_GAME for game %u (game over)\n", state->current_game_id);
                             }
                             state->current_game_id = 0;   // reset game id
                             state->screen = SCREEN_MAIN_MENU;
                             break;
                           }

    case SCREEN_SETTINGS: {
                            if (state->settings_mode == SETTINGS_EDIT) {
                              if (ch == '\n') {
                                switch (state->settings_active_field) {
                                  case 0:
                                    snprintf(state->config.username, sizeof(state->config.username), "%s", state->settings_edit_buf);
                                    break;
                                  case 1:
                                    snprintf(state->config.ip, sizeof(state->config.ip), "%s", state->settings_edit_buf);
                                    break;
                                  case 2:
                                    snprintf(state->config.port, sizeof(state->config.port), "%s", state->settings_edit_buf);
                                    break;
                                }
                                state->settings_mode = SETTINGS_NAV;
                              } else if (ch == 27) {
                                state->settings_mode = SETTINGS_NAV;
                              } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                                int len = (int)strlen(state->settings_edit_buf);
                                if (len > 0) {
                                  state->settings_edit_buf[len - 1] = '\0';
                                }
                              } else if (isprint(ch)) {
                                int len = (int)strlen(state->settings_edit_buf);
                                if (len < (int)sizeof(state->settings_edit_buf) - 1) {
                                  state->settings_edit_buf[len] = (char)ch;
                                  state->settings_edit_buf[len + 1] = '\0';
                                }
                              }
                            } else {
                              switch (ch) {
                                case KEY_UP:
                                  state->settings_active_field = (state->settings_active_field + 3) % 4;
                                  break;
                                case KEY_DOWN:
                                case '\t':
                                  state->settings_active_field = (state->settings_active_field + 1) % 4;
                                  break;
                                case '\n':
                                  if (state->settings_active_field == 3) {
                                    save_config(&state->config);
                                    state->screen = SCREEN_MAIN_MENU;
                                  } else {
                                    state->settings_mode = SETTINGS_EDIT;
                                    switch (state->settings_active_field) {
                                      case 0:
                                        strcpy(state->settings_edit_buf, state->config.username);
                                        break;
                                      case 1:
                                        strcpy(state->settings_edit_buf, state->config.ip);
                                        break;
                                      case 2:
                                        strcpy(state->settings_edit_buf, state->config.port);
                                        break;
                                    }
                                  }
                                  break;
                                case 27:
                                  state->screen = SCREEN_MAIN_MENU;
                                  break;
                              }
                            }
                            break;
                          }

    case SCREEN_HELP: {
                        state->screen = SCREEN_MAIN_MENU;
                        break;
                      }

    case SCREEN_DISCONNECTED:
                      // no input
                      break;
  }
}

// events from network
static void handle_event(UIState* state, UIEvent* ev) {
  switch (ev->type) {
    case UI_EVENT_GAME_STATE:
      state->game_state = ev->data.game_state.state_data;
      state->current_game_id = ev->data.game_state.game_id;
      state->screen = SCREEN_IN_GAME;
      break;
    case UI_EVENT_GAME_START:
      state->cursor_row = 0;
      state->cursor_col = 0;
      state->current_game_id = ev->data.game_start.game_id;
      state->player_role = (Player)ev->data.game_start.player_role;
      break;
    case UI_EVENT_GAME_OVER:
      state->screen = SCREEN_GAME_OVER;
      state->last_game_winner = ev->data.game_over.winner;   // store winner for YOU WIN/LOSE display
      snprintf(state->post_game_message, sizeof(state->post_game_message),
          "Game over: %s", ev->data.game_over.message);
      state->disconnected_at = time(NULL);
      break;
    case UI_EVENT_LOBBY_UPDATE:
      state->lobby_count = ev->data.lobby_update.game_count;
      for (int i = 0; i < state->lobby_count; i++) {
        state->lobby_games[i].game_id = ev->data.lobby_update.games[i].game_id;
        state->lobby_games[i].state = ev->data.lobby_update.games[i].state;

        strncpy(state->lobby_games[i].owner_name,
            ev->data.lobby_update.games[i].owner_name,
            sizeof(state->lobby_games[i].owner_name) - 1);
        state->lobby_games[i].owner_name[sizeof(state->lobby_games[i].owner_name) - 1] = '\0';
        state->lobby_games[i].players_connected = ev->data.lobby_update.games[i].players_connected;
      }
      break;
    case UI_EVENT_ERROR:
      strncpy(state->error_message, ev->data.error.error_message,
          sizeof(state->error_message) - 1);
      state->error_message[sizeof(state->error_message) - 1] = '\0';

      if (state->screen == SCREEN_GAME_WAITING) {
        state->screen = SCREEN_LOBBY;
      }

      break;
    case UI_EVENT_JOIN_REQUEST:
      strncpy(state->join_requester_name, ev->data.join_request.requesting_player_name,
          sizeof(state->join_requester_name) - 1);
      state->join_requester_name[sizeof(state->join_requester_name) - 1] = '\0';

      state->join_game_id = ev->data.join_request.game_id;
      state->screen = SCREEN_JOIN_REQUEST;
      break;
    case UI_EVENT_POST_GAME_OPTIONS:
      state->screen = SCREEN_POST_GAME;
      snprintf(state->post_game_message, sizeof(state->post_game_message), "Play again?");
      break;
    case UI_EVENT_DISCONNECTED:
      state->screen = SCREEN_DISCONNECTED;
      state->disconnected_at = time(NULL);
      break;
    case UI_EVENT_GAME_STATUS_CHANGE:
      state->current_game_id = ev->data.game_status_change.game_id;
      if (ev->data.game_status_change.new_state == GAME_WAITING) {
        state->screen = SCREEN_GAME_WAITING;
      }
      break;
    default:
      break;
  }
}

// main loop
void ui_run(queue_t* from_net, queue_t* to_net, Config* config) {
  init_ncurses();
  atomic_store(&ui_running, 1);

  UIState state;
  memset(&state, 0, sizeof(state));
  state.screen = SCREEN_MAIN_MENU;
  state.menu_selection = 0;
  state.net_ctx = NULL;
  state.is_connected = false;
  state.last_game_winner = RESULT_NONE;   // initialise winner
  if (config) {
    memcpy(&state.config, config, sizeof(Config));
  } else {
    // default fallback (should not happen)
    strcpy(state.config.username, "Player");
    strcpy(state.config.ip, "127.0.0.1");
    strcpy(state.config.port, "8080");
  }

  while (atomic_load(&ui_running)) {
    // events from network
    UIEvent* ev;
    while (queue_try_pop(from_net, (void**)&ev) == 0) {
      handle_event(&state, ev);
      free(ev);
    }

    // keyboard input
    int ch = getch();
    if (ch != ERR) {
      handle_keyboard(&state, ch, to_net, from_net);
    }

    // drawing
    switch (state.screen) {
      case SCREEN_MAIN_MENU:
        draw_main_menu(&state);
        break;
      case SCREEN_LOBBY:
        draw_lobby(&state);
        break;
      case SCREEN_GAME_WAITING:
        draw_game_waiting(&state);
        break;
      case SCREEN_IN_GAME:
        draw_game_board(&state);
        break;
      case SCREEN_JOIN_REQUEST:
        draw_join_request(&state);
        break;
      case SCREEN_POST_GAME:
        draw_post_game(&state);
        break;
      case SCREEN_DISCONNECTED:
        draw_disconnected(&state);
        if (time(NULL) - state.disconnected_at >= 2) {
          atomic_store(&ui_running, 0);
        }
        break;
      case SCREEN_SETTINGS:
        draw_settings(&state);
        break;
      case SCREEN_HELP:
        draw_help(&state);
        break;
      case SCREEN_GAME_OVER:
        draw_game_over(&state);
        break;
    }

    napms(50);
  }

  // cleanup
  if (state.net_ctx) {
    network_stop(state.net_ctx);
    network_wait(state.net_ctx);
    network_destroy(state.net_ctx);
  }

  UIEvent* ev;
  while (queue_try_pop(from_net, (void**)&ev) == 0) {
    free(ev);
  }

  endwin();
}
