#ifndef TUI_H
#define TUI_H

#include "scroll_view.h"
#include <ncurses.h>
#include <net/if.h>
#include <panel.h>
#include <pthread.h>
#include <stdint.h>

#define WINDOW_OUTER_INDENT 5
#define MIN_WIDTH 76
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

typedef struct {
	struct if_nameindex *items;
	uint32_t size;
	scroll_view view;
} interfaces_list;

void draw_window_frame(window_data *window_data, const char *title);
void input_handler(int32_t input, app_context *variables);
void draw(app_context *variables);
void resize_handler(app_context *variables);

#endif
