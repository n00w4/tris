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

void show_error_screen(char* error) {
  clear();
  mvprintw(LINES / 2 - 2, COLS / 2 - 15, "%s", error);
  refresh();

  clear();
  mvprintw(LINES / 2 - 3, COLS / 2 - 15, "Press a button to go back to the main menu");
  refresh();
  getch();
  return;
}

void show_create_game_screen(void) {
  if (check_connection() < 0) {
    return;
  }

  clear();

  char game_name[64] = {0};
  mvprintw(LINES / 2 - 2, COLS / 2 - 15, "Enter game name (optional): ");
  refresh();

  echo();
  mvgetnstr(LINES / 2, COLS / 2 - 15, game_name, sizeof(game_name) - 1);
  noecho();  Message req;
  req.type = MSG_CREATE_GAME;
  req.game_id = 0;

  if (send_message_to_server(&req) < 0) {
    mvprintw(LINES - 2, 2, "Error sending create game request.");
    refresh();
    getch();
    return;
  }

  mvprintw(LINES - 2, 2, "Request sent. Waiting for server confirmation...");
  refresh();

  Message response;
  if (receive_message_from_server(&response) < 0) {
    mvprintw(LINES - 2, 2, "Server not responding.");
    refresh();
    getch();
    return;
  }

  if (response.type == MSG_GAME_STATE) {
    mvprintw(LINES - 3, 2, "Game created successfully! ID: %u", response.game_id);
    mvprintw(LINES - 2, 2, "Waiting for players...");
    refresh();

    Message lobby_response;
    if (receive_message_from_server(&lobby_response) < 0) {
      mvprintw(LINES - 2, 2, "Error waiting for players.");
      refresh();
      getch();
      return;
    }

    if (lobby_response.type == MSG_GAME_STATE) {
      mvprintw(LINES - 2, 2, "Player joined! Starting game...");
      refresh();
      // TODO: call the function to play
      // show_game_screen(&lobby_response);
    } else if (lobby_response.type == MSG_ERROR) {
      mvprintw(LINES - 2, 2, "Error during lobby: %s", lobby_response.payload.error.error_message);
      refresh();
      getch();
    } else {
      mvprintw(LINES - 2, 2, "Unexpected server response during lobby.");
      refresh();
      getch();
    }
  } else if (response.type == MSG_ERROR) {
    mvprintw(LINES - 2, 2, "Create game failed: %s", response.payload.error.error_message);
    refresh();
    getch();
  } else {
    mvprintw(LINES - 2, 2, "Unexpected server response.");
    refresh();
    getch();
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
    mvprintw(LINES - 2, 2, "Error sending request to server.");
    refresh();
    getch();
    return;
  }

  Message response;
  if (receive_message_from_server(&response) < 0 || response.type != MSG_GAME_LIST) {
    mvprintw(LINES - 2, 2, "Error receiving game list from server.");
    refresh();
    getch();
    return;
  }

  GameListPayload* payload = &response.payload.game_list;

  if (payload->game_count == 0) {
    mvprintw(LINES / 2, COLS / 2 - 10, "No games available.");
    refresh();
    getch();
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
      mvprintw(LINES - 2, 2, "Error sending join request.");
      refresh();
      getch();
      return;
    }

    Message reply;
    if (receive_message_from_server(&reply) < 0) {
      mvprintw(LINES - 2, 2, "Server not responding.");
      refresh();
      getch();
      return;
    }

    if (reply.type == MSG_GAME_STATE) {
      mvprintw(LINES - 2, 2, "Joined game successfully. Starting game...");
      refresh();
      getch();
      // TODO: call the function to play
      // show_game_screen(&reply);
    } else if (reply.type == MSG_ERROR) {
      mvprintw(LINES - 2, 2, "Join request rejected: %s", reply.payload.error.error_message);
      refresh();
      getch();
    } else {
      mvprintw(LINES - 2, 2, "Unexpected server response.");
      refresh();
      getch();
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

