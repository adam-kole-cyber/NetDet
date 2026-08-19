#ifndef TUI_APP_H
#define TUI_APP_H

#include "device.h"
#include <ncurses.h>
#include <stdint.h>

typedef struct {
	int32_t col_width;
	int32_t col_width_remainder;
} table_layout;

void draw_table_header(WINDOW *window);
void print_network_data(WINDOW *window, sliding_window_buffer *buffer);
void set_column_width(int32_t window_width);

#endif
