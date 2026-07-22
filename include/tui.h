#ifndef TUI_H
#define TUI_H

#include "device.h"
#include <ncurses.h>
#include <stdint.h>

#define WINDOW_OUTER_INDENT 5
#define MIN_WIDTH 83
#define MIN_HEIGHT 4
#define WINDOW_TIMEOUT 100
#define WINDOW_UNUSABLE_NUMBERS_OF_LINES 2 // they are unusable because of the window frame

extern sliding_window_buffer buffer;
extern hash_map map;
extern pthread_mutex_t device_data_structures_mutex;

typedef struct {
	int32_t width;
	int32_t height;
	int32_t start_x;
	int32_t start_y;
	WINDOW *window;
} window_data;

void ncurses_init(void);
void draw_window_frame(window_data *window_data, const char *title);
void input_handler(window_data *window_data, int32_t input);
void draw_table_header(WINDOW *window);
void print_network_data(WINDOW *window);

#endif
