#include "ui/ui.h"
#include "utils/utils.h"
#include "ui/menu.h"
#include "ui/screens.h"

#include <ncurses.h>
#include <string.h>

int show_main_menu(void) {
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

  menu_action_t actions[] = {
    show_create_game_screen,
    show_join_game_screen,
    show_settings_menu,
    show_help_screen
  };

  int choice = handle_generic_menu_input(&main_menu, true);
  cleanup_generic_menu(&main_menu);
  clear();

  if (choice == 4 || choice == -1) {
    return -1; 
  }

  if (choice >= 0 && choice < 4) {
    actions[choice](); 
  }

  clear();
  refresh();
  return 0;
}

static void clear_field_area(int y, int x, int width) {
  mvhline(y, x, ' ', width);
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

  clear_field_area(y, x + strlen(label), max_width);

  const char* display_value = value[0] == '\0' ? " <not set> " : value;
  mvprintw(y, x + strlen(label), "%-*s", max_width, display_value);

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
  bool exit_menu = false;

  // Where data and fields are located
  struct {
    int offset_y;
    char* buffer;
    int size;
  } fields[] = {
    {2, config.username, 18},
    {4, config.ip,       15},
    {6, config.port,     5}
  };

  while (!exit_menu) {
    clear();
    attron(A_BOLD);
    mvprintw(menu_y, menu_x + (menu_width - 16) / 2, "=== SETTINGS ===");
    attroff(A_BOLD);

    draw_settings_fields(menu_y, menu_x, &config, active_field);
    refresh();

    int ch = getch();
    switch (ch) {
      case KEY_UP:    active_field = (active_field + 3) % 4; break;
      case KEY_DOWN:
      case '\t':      active_field = (active_field + 1) % 4; break;

      case 10: // ENTER
        if (active_field < 3) {
          int field_y = menu_y + fields[active_field].offset_y;
          const char* label = NULL;
          switch (active_field) {
            case 0: label = "Username: "; break;
            case 1: label = "IP Server: "; break;
            case 2: label = "Port: "; break;
          }
          int field_x = menu_x + 2 + strlen(label);
          int field_width = fields[active_field].size;
          get_styled_input(field_y, field_x, fields[active_field].buffer, field_width);
          } else {
            save_config(&config);
            mvprintw(menu_y + 10, menu_x + 5, "Settings saved!");
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
}

