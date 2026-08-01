#ifndef TUI_APP_H
#define TUI_APP_H

#include "device.h"
#include <ncurses.h>

void draw_table_header(WINDOW *window);
void print_network_data(WINDOW *window, sliding_window_buffer *buffer);

#endif
