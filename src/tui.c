#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include "tui.h"

void ncurses_init(void){
	setlocale(LC_ALL, "");
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(0);

	if (has_colors()){	// enables colors in terminal
		start_color();
		use_default_colors();
	}
}

void draw_window_frame(WINDOW *window, int window_width, int window_height){
	mvwprintw(window, 0, 0, "╭");		// draw the corners of the frame
	mvwprintw(window, 0, window_width - 1, "╮");
	mvwprintw(window, window_height - 1, 0, "╰");
	mvwprintw(window, window_height - 1, window_width - 1, "╯");

	for (int x = 1; x < (window_width - 1); x++){	// connects the corners horizontally
		mvwprintw(window, 0, x, "─");
		mvwprintw(window, window_height - 1, x, "─");
	}

	for (int y = 1; y < (window_height - 1); y++){	// connects the corners vertically 
		mvwprintw(window, y, 0, "│");
		mvwprintw(window, y, window_width - 1, "│");
	}
}
