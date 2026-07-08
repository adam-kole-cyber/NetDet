#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
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
		init_pair(1, COLOR_GREEN, -1);
	}
}

void draw_window_frame(WINDOW *window, int window_width, int window_height, const char *title){
	bool title_set = title != NULL;
	int last_usable_column = window_width - 1;	// the window width will be reduced due to the frame
	int last_usable_row = window_height - 1;
	int minimum_title_area = last_usable_column - 2;	// to ensure that title is not sticked together with corners

	mvwprintw(window, 0, 0, "╭");		// draw the corners of the frame
	mvwprintw(window, 0, last_usable_column, "╮");
	mvwprintw(window, last_usable_row, 0, "╰");
	mvwprintw(window, last_usable_row, last_usable_column, "╯");

	for (int x = 1; x < (last_usable_column); x++){	// connects the corners horizontally
		mvwprintw(window, 0, x, "─");
		mvwprintw(window, last_usable_row, x, "─");
	}

	if (title_set && (int)strlen(title) < minimum_title_area){
		wattron(window, COLOR_PAIR(1));
		mvwprintw(window, 0, 2, "%s", title);
		wattroff(window, COLOR_PAIR(1));
	}

	for (int y = 1; y < (last_usable_row); y++){	// connects the corners vertically 
		mvwprintw(window, y, 0, "│");
		mvwprintw(window, y, last_usable_column, "│");
	}
}
