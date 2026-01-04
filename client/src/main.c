#include <ncurses.h>
#include <stdbool.h>

#include "ui/ui.h"

int main(void) {
  init_ui();
  show_main_menu();
  getch();
  cleanup_ui();
  return 0;
}
