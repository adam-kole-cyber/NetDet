#ifndef TUI_H
#define TUI_H
#include <ncurses.h>

#define WINDOW_OUTER_INDENT 5

typedef struct {
	int width;
	int height;
	int start_x;
	int start_y;
	WINDOW *window;
} window_data;

void ncurses_init(void);
void draw_window_frame(window_data *window_data, const char *title);
void input_handler(window_data *window_data, int input);
void resize_handler(window_data *window_data);
void mvwprintIPw(window_data *window_data, int y, int x, const unsigned char *ip);

#endif
