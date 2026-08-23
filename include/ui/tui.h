#ifndef UI_TUI_H
#define UI_TUI_H

#include <ncurses.h>
#include <panel.h>
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

void ncurses_init(void);
void draw_window_frame(window_data *window_data, const char *title);
void input_handler(int32_t input, app_context *variables);
void draw(app_context *variables);
void resize_handler(app_context *variables);

#endif
