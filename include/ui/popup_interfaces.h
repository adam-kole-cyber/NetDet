#ifndef UI_POPUP_INTERFACES_H
#define UI_POPUP_INTERFACES_H

#include <ncurses.h>
#include <stdint.h>

extern struct if_nameindex *popup_list;

uint32_t get_interface_index(int32_t interface_index);
char *get_interface_name(int32_t interface_index);
void prepare_interface_list(void *args);
void render_interface_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data);
void interfaces_list_clean_up(void *args);

#endif
