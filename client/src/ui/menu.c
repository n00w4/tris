#include "ui.h"
#include "../utils/utils.h"
#include "menu.h"

#include <ncurses.h>
#include <string.h>

void show_main_menu(void) {
  clear();

  int menu_height = 16;
  int menu_width = 50;
  int menu_y = (LINES - menu_height) / 2;
  int menu_x = (COLS - menu_width) / 2;

  Menu main_menu;
  init_generic_menu(&main_menu, "=== TRIS CLIENT ===", menu_y, menu_x, menu_height, menu_width);

  add_menu_item(&main_menu, "Create new game");
  add_menu_item(&main_menu, "Join a game");
  add_menu_item(&main_menu, "Settings");
  add_menu_item(&main_menu, "Help");
  add_menu_item(&main_menu, "Exit");

  int choice = handle_generic_menu_input(&main_menu);
  cleanup_generic_menu(&main_menu);
  clear();

  switch (choice) {
    case 0: // Create new game
      mvprintw(LINES/2, COLS/2 - 10, "Creating new game...");
      getch();
      break;
    case 1: // Join a game
      mvprintw(LINES/2, COLS/2 - 8, "Joining game...");
      getch();
      break;
    case 2: // Settings
      show_settings_menu();
      break;
    case 3: // Help
      mvprintw(LINES/2, COLS/2 - 5, "Help screen...");
      getch();
      break;
    case 4: // Exit
    case -1:
      mvprintw(LINES/2, COLS/2 - 5, "Exiting...");
      break;
  }

  refresh();
}

static void clear_field_area(int y, int x, int width) {
  mvprintw(y, x, "%*s", width, "");
  refresh();
}

static void print_field(int y, int x, const char* label, const char* value, int active, int field_index, int max_width) {
  attron(A_BOLD);
  mvprintw(y, x, "%s", label);
  attroff(A_BOLD);

  attrset(A_NORMAL);

  if (active == field_index) {
    attron(A_REVERSE);
  } else {
    attron(A_DIM);
  }

  mvprintw(y, x + strlen(label), "%-*s", max_width, value[0] == '\0' ? "[Enter value]" : value);

  attroff(A_REVERSE | A_DIM);
}

void draw_settings_fields(int y, int x, const Config* config, int active_field) {
  print_field(y + 2, x + 2, "Username: ", config->username, active_field, 0, 18);
  print_field(y + 4, x + 2, "IP Server: ", config->ip, active_field, 1, 15);
  print_field(y + 6, x + 2, "Port: ", config->port, active_field, 2, 5);

  // SAVE
  attrset(A_NORMAL);
  if (active_field == 3) {
    attron(A_REVERSE | A_BOLD);
  } else {
    attron(A_BOLD | A_DIM);
  }
  mvprintw(y + 12, x + 15, "  [ SAVE ]  ");
  attroff(A_REVERSE | A_BOLD | A_DIM);

  attrset(A_NORMAL);
  attron(A_DIM);
  mvprintw(y + 10, x + 5, "ENTER: Edit/Select  ESC: Exit  TAB: Next field");
  attroff(A_DIM);
}

void show_settings_menu(void) {
  int menu_height = 12, menu_width = 40;
  int menu_y = (LINES - menu_height) / 2;
  int menu_x = (COLS - menu_width) / 2;

  Config config = {0};
  read_config(&config);

  int active_field = 0;
  int ch;
  bool exit_menu = false;

  while (!exit_menu) {
    clear();

    attron(A_BOLD);
    mvprintw(menu_y, menu_x + (menu_width - 16) / 2, "=== SETTINGS ===");
    attroff(A_BOLD);

    draw_settings_fields(menu_y, menu_x, &config, active_field);
    refresh();

    ch = getch();
    switch (ch) {
      case KEY_UP:
        active_field = (active_field + 3) % 4;
        break;
      case KEY_DOWN:
      case '\t':
        active_field = (active_field + 1) % 4;
        break;
      case 10: // Enter
        if (active_field == 0) {
          clear_field_area(menu_y + 2, menu_x + 14, 18);
          get_styled_input(menu_y + 2, menu_x + 14, config.username, sizeof(config.username) - 1);
        } else if (active_field == 1) {
          clear_field_area(menu_y + 4, menu_x + 14, 15);
          get_styled_input(menu_y + 4, menu_x + 14, config.ip, sizeof(config.ip) - 1);
        } else if (active_field == 2) {
          clear_field_area(menu_y + 6, menu_x + 14, 5);
          get_styled_input(menu_y + 6, menu_x + 14, config.port, sizeof(config.port) - 1);
        } else if (active_field == 3) {
          save_config(&config);
          mvprintw(menu_y + 14, menu_x + 5, "Settings saved successfully!");
          refresh();
          napms(1000);
          exit_menu = true;
        }
        break;
      case 27: // ESC
        exit_menu = true;
        break;
    }
  }

  if (exit_menu) { 
    clear();
    refresh();
    show_main_menu();
  }
}

