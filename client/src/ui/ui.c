#include "ui/ui.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static WINDOW* main_win = NULL;
static WINDOW* status_win = NULL;
static WINDOW* menu_win = NULL;

void init_ui(void) {
  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  menu_win = newwin(LINES - 4, COLS, 0, 0);
  keypad(menu_win, TRUE);

  status_win = newwin(3, COLS, LINES - 3, 0);
  main_win = stdscr;
}

void cleanup_ui(void) {
  if (menu_win) { delwin(menu_win); }
  if (status_win) { delwin(status_win); }
  endwin();
}

void init_menu(Menu* menu, const char* title, int start_y, int start_x, int height, int width) {
  if (height < 8) height = 8;
  if (width < 30) width = 30;
  
  if (start_y + height > LINES) {
    height = LINES - start_y - 1;
  }
  if (start_x + width > COLS) {
    width = COLS - start_x - 1;
  }
  
  menu->title = strdup(title);
  menu->item_count = 0;
  menu->selected_index = 0;
  menu->start_y = start_y;
  menu->start_x = start_x;
  menu->height = height;
  menu->width = width;
  menu->border = true;
  
  menu->win = newwin(height, width, start_y, start_x);
  keypad(menu->win, TRUE);
  
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
  }
}

void init_generic_menu(Menu* menu, const char* title, int start_y, int start_x, int height, int width) {
  if (height < 8) height = 8;
  if (width < 30) width = 30;
  
  if (start_y + height > LINES) {
    height = LINES - start_y - 1;
  }
  if (start_x + width > COLS) {
    width = COLS - start_x - 1;
  }
  
  menu->title = strdup(title);
  menu->item_count = 0;
  menu->selected_index = 0;
  menu->start_y = start_y;
  menu->start_x = start_x;
  menu->height = height;
  menu->width = width;
  menu->border = true;
  
  menu->win = newwin(height, width, start_y, start_x);
  keypad(menu->win, TRUE);
}

void draw_generic_menu(Menu* menu) {
  werase(menu->win);
  
  if (menu->border) {
    box(menu->win, 0, 0);
    
    if (menu->width > 4 && menu->height > 4) {
      mvwaddch(menu->win, 0, 0, ACS_ULCORNER);
      mvwaddch(menu->win, 0, menu->width - 1, ACS_URCORNER);
      mvwaddch(menu->win, menu->height - 1, 0, ACS_LLCORNER);
      mvwaddch(menu->win, menu->height - 1, menu->width - 1, ACS_LRCORNER);
    }
  }
  
  int title_len = strlen(menu->title);
  int title_x = (menu->width - title_len) / 2;
  if (title_x < 2) { title_x = 2; }
  
  wattron(menu->win, A_BOLD | A_UNDERLINE);
  mvwprintw(menu->win, 2, title_x, "%s", menu->title);
  wattroff(menu->win, A_BOLD | A_UNDERLINE);
  
  if (menu->width > 4) {
    for (int i = 2; i < menu->width - 2; i++) {
      mvwaddch(menu->win, 3, i, ACS_HLINE);
    }
    mvwaddch(menu->win, 3, 1, ACS_LTEE);
    mvwaddch(menu->win, 3, menu->width - 2, ACS_RTEE);
  }
  
  int start_y = 5;
  int max_items_to_show = menu->height - start_y - 3;
  
  for (int i = 0; i < menu->item_count && i < max_items_to_show; i++) {
    int item_y = start_y + i;
    
    int item_len = strlen(menu->items[i]);
    int item_x = (menu->width - item_len) / 2;
    if (item_x < 4) { item_x = 4; }
    
    if (i == menu->selected_index) {
      wattron(menu->win, A_REVERSE);
      mvwprintw(menu->win, item_y, item_x - 2, "> %s <", menu->items[i]);
      wattroff(menu->win, A_REVERSE);
    } else {
      mvwprintw(menu->win, item_y, item_x, "  %s  ", menu->items[i]);
    }
  }
  
  if (menu->height > 2) {
    wattron(menu->win, A_DIM);
    
    const char* instruction1 = "Use arrows to navigate";
    int inst1_x = (menu->width - strlen(instruction1)) / 2;
    if (inst1_x < 2) inst1_x = 2;
    mvwprintw(menu->win, menu->height - 4, inst1_x, "%s", instruction1);
    
    const char* instruction2 = "Press ENTER to select";
    int inst2_x = (menu->width - strlen(instruction2)) / 2;
    if (inst2_x < 2) inst2_x = 2;
    mvwprintw(menu->win, menu->height - 3, inst2_x, "%s", instruction2);
    
    const char* instruction3 = "ESC to exit";
    int inst3_x = (menu->width - strlen(instruction3)) / 2;
    if (inst3_x < 2) inst3_x = 2;
    mvwprintw(menu->win, menu->height - 2, inst3_x, "%s", instruction3);
    
    wattroff(menu->win, A_DIM);
  }
  
  wrefresh(menu->win);
}

void add_menu_item(Menu* menu, const char* item) {
  if (menu->item_count < MAX_MENU_ITEMS) {
    menu->items[menu->item_count] = strdup(item);
    menu->item_count++;
  }
}

void cleanup_generic_menu(Menu* menu) {
  free(menu->title); 
  for (int i = 0; i < menu->item_count; i++) {
    free(menu->items[i]);
  }
  if (menu->win) {
    delwin(menu->win);
  }
}

int handle_generic_menu_input(Menu* menu, bool is_main_menu) {
  int ch;
  
  while (1) {
    draw_generic_menu(menu);
    ch = wgetch(menu->win);

    switch (ch) {
      case KEY_UP:
        if (menu->selected_index > 0) menu->selected_index--;
        break;
      case KEY_DOWN:
        if (menu->selected_index < menu->item_count - 1) menu->selected_index++;
        break;
      case '\n':
        return menu->selected_index;
      case 27:
        return is_main_menu ? MENU_QUIT : MENU_BACK;
      case KEY_RESIZE:
        clear();
        refresh();
        break;
    }
  }
}

void get_styled_input(int y, int x, char *buffer, int max_len) {
    curs_set(1);
    echo();
    
    attron(A_REVERSE);
    mvprintw(y, x, "%-*s", max_len, " ");
    move(y, x);
    attroff(A_REVERSE);
    refresh();
    
    getnstr(buffer, max_len);
    
    noecho();
    curs_set(0);
}

