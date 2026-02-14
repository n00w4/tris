#include "ui/screens.h"
#include "network/network.h"
#include "ui/ui.h"

#include <ncurses.h>
#include <stdio.h>

int check_connection(void) {
  if (connect_with_config() < 0) {
    show_error_screen("Cannot connect to server. Check settings.");
    return -1;
  }
  return 0;
}

void show_error_screen(const char* error) {
  clear();
  attron(A_BOLD);
  mvprintw(LINES / 2 - 6, COLS / 2 - 15, "================ ERROR ===============");
  attroff(A_BOLD);
  mvprintw(LINES / 2 - 3, COLS / 2 - 15, "%s", error);
  mvprintw(LINES / 2 - 1, COLS / 2 - 15, "Press a button to go back to the main menu");
  refresh();
  getch();
  return;
}

void show_info_screen(const char* info) {
  clear();
  attron(A_BOLD);
  mvprintw(LINES / 2 - 6, COLS / 2 - 15, "================ INFO ===============");
  attroff(A_BOLD);

  move(LINES / 2 - 3, COLS / 2 - 15);
  mvprintw(LINES / 2 - 3, COLS / 2 - 15, "%s", info);
  refresh();
  return;
}

void show_create_game_screen(void) {
  if (check_connection() < 0) {
    return;
  }

  curs_set(1);
  clear();

  char game_name[64] = {0};
  mvprintw(LINES / 2 - 2, COLS / 2 - 15, "Enter game name (optional): ");
  refresh();

  echo();
  mvgetnstr(LINES / 2, COLS / 2 - 15, game_name, sizeof(game_name) - 1);
  noecho();
  curs_set(0);

  Message req;
  req.type = MSG_CREATE_GAME;
  req.game_id = 0;

  if (send_message_to_server(&req) < 0) {
    show_error_screen("Error sending create game request.");
    return;
  }

  show_info_screen("Request sent. Waiting for server confirmation...");

  Message response;
  if (receive_message_from_server(&response) < 0) {
    show_error_screen("Server not responding.");
    return;
  }

  if (response.type == MSG_GAME_STATE) {
    show_info_screen("Waiting for players...");
    mvprintw(LINES - 2, 2, "Game created successfully! ID: %u", response.game_id);
    refresh();

    Message lobby_response;
    if (receive_message_from_server(&lobby_response) < 0) {
      show_error_screen("Error waiting for players.");
      return;
    }

    if (lobby_response.type == MSG_GAME_STATE) {
      mvprintw(LINES - 2, 2, "Player joined! Starting game...");
      refresh();
      // TODO: call the function to play
      // show_game_screen(&lobby_response);
    } else if (lobby_response.type == MSG_ERROR) {
      show_error_screen(lobby_response.payload.error.error_message);
    } else {
      show_error_screen("Unexpected server response during lobby.");
    }
  } else if (response.type == MSG_ERROR) {
    show_error_screen(response.payload.error.error_message);
  } else {
    show_error_screen("Unexpected server response.");
  }
}

void show_join_game_screen(void) {
  if (check_connection() < 0) {
    return;
  }

  Message req;
  req.type = MSG_LIST_GAMES;
  req.game_id = 0;

  if (send_message_to_server(&req) < 0) {
    show_error_screen("Error sending request to server.");
    return;
  }

  Message response;
  if (receive_message_from_server(&response) < 0 || response.type != MSG_GAME_LIST) {
    show_error_screen("Error receiving game list from server.");
    return;
  }

  GameListPayload* payload = &response.payload.game_list;

  if (payload->game_count == 0) {
    show_info_screen("No games available.");
    return;
  }

  int menu_height = payload->game_count + 12;
  int menu_width = 50;
  int menu_y = (LINES - menu_height) / 2;
  int menu_x = (COLS - menu_width) / 2;

  Menu join_menu;
  init_generic_menu(&join_menu, "JOIN A GAME", menu_y, menu_x, menu_height, menu_width);

  for (int i = 0; i < payload->game_count; i++) {
    char item[64];
    snprintf(item, sizeof(item), "Game ID: %u | State: %s", payload->game_ids[i],
        payload->game_states[i] == 0 ? "New" : payload->game_states[i] == 1 ? "Waiting" : "In Progress");
    add_menu_item(&join_menu, item);
  }

  draw_generic_menu(&join_menu);
  refresh();

  int choice = handle_generic_menu_input(&join_menu, false);

  cleanup_generic_menu(&join_menu);

  if (choice == MENU_BACK) {
    return;
  }

  if (choice >= 0 && choice < payload->game_count) {
    Message join_msg;
    join_msg.type = MSG_JOIN_GAME;
    join_msg.game_id = payload->game_ids[choice];

    if (send_message_to_server(&join_msg) < 0) {
      show_error_screen("Error sending join request.");
      return;
    }

    Message reply;
    if (receive_message_from_server(&reply) < 0) {
      show_error_screen("Server not responding.");
      return;
    }

    if (reply.type == MSG_GAME_STATE) {
      show_info_screen("Joined game successfully. Starting game...");
      // TODO: call the function to play
      // show_game_screen(&reply);
    } else if (reply.type == MSG_ERROR) {
      show_error_screen(reply.payload.error.error_message);
    } else {
      show_error_screen("Unexpected server response.");
    }
  }
}

void show_help_screen(void) {
  clear();

  attron(A_BOLD);
  mvprintw(1, COLS / 2 - 10, "=== TRIS HELP ===");
  attroff(A_BOLD);

  int y = 4;
  mvprintw(y++, 2, "Goal of the game:");
  mvprintw(y++, 4, "- Align three identical symbols in a row (horizontal, vertical, or diagonal).");
  y++;

  mvprintw(y++, 2, "How to play:");
  mvprintw(y++, 4, "- Use the ARROW KEYS to navigate through the menus.");
  mvprintw(y++, 4, "- Press ENTER to select an option.");
  mvprintw(y++, 4, "- Press ESC to go back (except in the main menu, where ESC exits the app).");
  mvprintw(y++, 4, "- During the game, you can also use LEFT CLICK to select a cell.");
  y++;

  mvprintw(y++, 2, "Navigation:");
  mvprintw(y++, 4, "- All menus follow the same pattern: arrows to move, enter to confirm.");
  y++;

  mvprintw(y++, 2, "Press any key to return to the main menu...");

  refresh();
  getch();
}

