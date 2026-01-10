#include "ui/ui.h"
#include "ui/menu.h"

#include <ncurses.h>
#include <stdbool.h>

int main(void) {
  init_ui();
  show_main_menu();
  cleanup_ui();
  return 0;
}
