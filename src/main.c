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
	(void)argc;
	(void)argv;

	struct sigaction sa;
	sa.sa_handler = SIGINT_handler;
	sigfillset(&sa.sa_mask);		// suppress all signals to ensure the program terminates correctly
	sigaction(SIGINT, &sa, NULL);

	ncurses_init();

	int input = 0;

	pthread_t network_thread;

	window_data main_window;
	main_window.start_x = WINDOW_OUTER_INDENT;
	main_window.start_y = WINDOW_OUTER_INDENT;
	main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	main_window.window = newwin(main_window.height, main_window.width, main_window.start_y, main_window.start_x);

	pthread_create(&network_thread, NULL, network_routine, NULL);

	do {
		werase(main_window.window);
		draw_window_frame(&main_window, " NetDet ");
		wrefresh(main_window.window);
		
		input = wgetch(main_window.window);
		input_handler(&main_window, input);

	} while(!end_program);
	
	pthread_join(network_thread, NULL);

	delwin(main_window.window);
	endwin();
	return 0;
}
