#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "network.h"
#include "tui.h"

pthread_t main_thread_id;
volatile sig_atomic_t end_main_loop = 0;

void sigint_handler(int arg){
	(void)arg;
	end_main_loop = 1;
}

void sigusr1_handler(int arg){
	(void)arg;
	end_main_loop = 1;
}

int main(int argc, char *argv[]){
	(void)argc;
	(void)argv;
	main_thread_id = pthread_self();

	struct sigaction sigint_action;
	sigint_action.sa_handler = sigint_handler;
	sigfillset(&sigint_action.sa_mask);		// suppress all signals to ensure the program terminates correctly
	sigaction(SIGINT, &sigint_action, NULL);

	struct sigaction sigusr1_action;
	sigusr1_action.sa_handler = sigusr1_handler;
	sigfillset(&sigusr1_action.sa_mask);
	sigaction(SIGUSR1, &sigusr1_action, NULL);

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

	} while(!end_main_loop);
	
	pthread_join(network_thread, NULL);

	delwin(main_window.window);
	endwin();
	return 0;
}
