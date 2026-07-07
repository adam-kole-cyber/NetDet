#ifndef TUI_H
#define TUI_H
#include <ncurses.h>

void ncurses_init(void); 
void draw_window_frame(WINDOW *window, int window_width, int window_height);

#endif
