#include "error.h"
#include "network.h"
#include "signal_handler.h"
#include "tui.h"
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

pthread_t main_thread_id;
volatile sig_atomic_t end_main_loop = 0;
volatile sig_atomic_t termination_reason = PROGRAM_RUNNING;

int main(int argc, char *argv[]) {
	main_thread_id = pthread_self();

	struct sigaction sigint_action = {0};
	sigint_action.sa_handler = sigint_handler;
	sigfillset(&sigint_action.sa_mask); // suppress all signals to ensure the program terminates correctly
	sigaction(SIGINT, &sigint_action, NULL);

	struct sigaction sigusr1_action = {0};
	sigusr1_action.sa_handler = sigusr1_handler;
	sigfillset(&sigusr1_action.sa_mask);
	sigaction(SIGUSR1, &sigusr1_action, NULL);

	pthread_t network_thread;
	struct network_thread_args args;
	args.argc = argc;
	args.argv = argv;

	pthread_create(&network_thread, NULL, network_routine, (void *)&args);

	ncurses_init();

	int input = 0;

	window_data main_window;
	main_window.start_x = WINDOW_OUTER_INDENT;
	main_window.start_y = WINDOW_OUTER_INDENT;
	main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	main_window.window = newwin(main_window.height, main_window.width, main_window.start_y, main_window.start_x);
	wtimeout(main_window.window, 100);

	while (!end_main_loop) {
		werase(main_window.window);
		draw_window_frame(&main_window, " NetDet ");
		wrefresh(main_window.window);

		input = wgetch(main_window.window);
		input_handler(&main_window, input);
	}

	pthread_join(network_thread, NULL);

	delwin(main_window.window);
	endwin();

	get_error();
	return 0;
}
