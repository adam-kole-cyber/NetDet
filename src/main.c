#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "network.h"
#include "tui.h"

volatile sig_atomic_t end_program = 0;

void SIGINT_handler(int arg){
	(void)arg;
	end_program = 1;
}

int main(int argc, char *argv[]){

	struct sigaction sa;
	sa.sa_handler = SIGINT_handler;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

	ncurses_init();

	int input = 0;
	int window_outer_indent = 5;
	int window_start_y = window_outer_indent;
	int window_start_x = window_outer_indent;
	int window_width = COLS - (window_outer_indent * 2);	// subtract the window's outer indentation from the total number of columns
	int window_height = LINES - (window_outer_indent * 2);	// subtract the window's outer indentation from the total number of lines
	WINDOW *main_window = newwin(window_height, window_width, window_start_y, window_start_x);

	do {
		werase(main_window);
		draw_window_frame(main_window, window_width, window_height);
		wrefresh(main_window);
		input = wgetch(main_window);
	} while(!end_program);

	delwin(main_window);
	endwin();
	return 0;
}
