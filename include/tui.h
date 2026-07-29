#ifndef TUI_H
#define TUI_H

#include "device.h"
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>
#include <stdint.h>

#define WINDOW_OUTER_INDENT 5
#define MIN_WIDTH 83
#define MIN_HEIGHT 4
#define WINDOW_UNUSABLE_NUMBERS_OF_LINES 2 // they are unusable because of the window frame

typedef struct app_context app_context;

typedef struct {
	bool is_active;
	int32_t width;
	int32_t height;
	int32_t start_x;
	int32_t start_y;
	WINDOW *window;
	PANEL *panel;
} window_data;

void ncurses_init(void);
void draw_window_frame(window_data *window_data, const char *title);
void input_handler(int32_t input, app_context *variables);
void draw_table_header(WINDOW *window);
void print_network_data(WINDOW *window, sliding_window_buffer *buffer);
void draw(app_context *variables);
void popup_window_action(window_data *main_window, window_data *popup_window, pthread_t signal_thread);
void draw_popup(window_data *popup_window);

#endif
