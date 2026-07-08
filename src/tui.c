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

void draw_window_frame(window_data *window_data, const char *title){
	bool title_set = title != NULL;
	int last_usable_column = window_data->width - 1;	// the window width will be reduced due to the frame
	int last_usable_row = window_data->height - 1;
	int minimum_title_area = last_usable_column - 2;	// to ensure that title is not sticked together with corners

	mvwprintw(window_data->window, 0, 0, "╭");		// draw the corners of the frame
	mvwprintw(window_data->window, 0, last_usable_column, "╮");
	mvwprintw(window_data->window, last_usable_row, 0, "╰");
	mvwprintw(window_data->window, last_usable_row, last_usable_column, "╯");

	for (int x = 1; x < (last_usable_column); x++){	// connects the corners horizontally
		mvwprintw(window_data->window, 0, x, "─");
		mvwprintw(window_data->window, last_usable_row, x, "─");
	}

	if (title_set && (int)strlen(title) < minimum_title_area){
		wattron(window_data->window, COLOR_PAIR(1));
		mvwprintw(window_data->window, 0, 2, "%s", title);
		wattroff(window_data->window, COLOR_PAIR(1));
	}

	for (int y = 1; y < (last_usable_row); y++){	// connects the corners vertically 
		mvwprintw(window_data->window, y, 0, "│");
		mvwprintw(window_data->window, y, last_usable_column, "│");
	}
}

void input_handler(window_data *window_data, int input){
	switch (input){
		case KEY_RESIZE:
			resize_handler(window_data);
			break;
		default:
			break;
	}
}

void resize_handler(window_data *window_data){
	int new_height = LINES - (WINDOW_OUTER_INDENT * 2);
	int new_width = COLS - (WINDOW_OUTER_INDENT * 2);
	
	for (int i = 0; i < window_data->height; i++){	// cleaning leftovers from old window
		for(int j = 0; j < window_data->width; j++){
			mvwprintw(window_data->window, i, j, " ");
		}
	}
	
	window_data->height = new_height;
	window_data->width = new_width;

	wnoutrefresh(window_data->window);
    doupdate();
	
	wresize(window_data->window, new_height, new_width);
	mvwin(window_data->window, WINDOW_OUTER_INDENT, WINDOW_OUTER_INDENT);
}
