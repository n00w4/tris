#ifndef MENU_H
#define MENU_H

#include "../utils/utils.h"

typedef void (*menu_action_t)(void);

int show_main_menu(void);
void show_settings_menu(void);
void draw_settings_fields(int y, int x, const Config* config, int active_field);

#endif
