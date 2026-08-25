#ifndef UI_TUI_APP_H
#define UI_TUI_APP_H

#include "core/device.h"
#include "scroll_view.h"
#include <ncurses.h>
#include <stdint.h>

typedef struct {
	int32_t col_width;
	int32_t col_width_remainder;
} table_layout;

typedef struct {
	device_buffer *data;
	scroll_view view;
} device_table_view;

void draw_table_header(WINDOW *window);
void print_network_data(WINDOW *window, device_table_view *buffer);
void set_column_width(int32_t window_width);
void update_row(void);

#endif
