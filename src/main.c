#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "network.h"
#include "tui.h"

int main(int argc, char *argv[]){
	ncurses_init();

	int window_outer_indent = 5;
	int window_start_y = window_outer_indent;
	int window_start_x = window_outer_indent;
	int window_width = COLS - (window_outer_indent * 2);	// subtract the window's outer indentation from the total number of columns
	int window_height = LINES - (window_outer_indent * 2);	// subtract the window's outer indentation from the total number of lines
	WINDOW *main_window = newwin(window_height, window_width, window_start_y, window_start_x);

	draw_window_frame(main_window, window_width, window_height);
	mvwprintw(main_window, 5, 0, "Skuska okna");
	wrefresh(main_window);

	while(1);

	delwin(main_window);
	endwin();
	return 0;
}
