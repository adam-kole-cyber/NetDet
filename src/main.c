#include "error.h"
#include "network.h"
#include "signal_handler.h"
#include "tui.h"
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/eventfd.h>
#include <unistd.h>

int shutdown_fd;
atomic_bool end_main_loop = false;
atomic_uint_fast32_t termination_reason = PROGRAM_RUNNING;
ringbuffer *ring_buffer;

int main(int argc, char *argv[]) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	shutdown_fd = eventfd(0, 0);

	pthread_t signal_thread;
	pthread_create(&signal_thread, NULL, signal_routine, NULL);

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

	while (!atomic_load(&end_main_loop)) {
		werase(main_window.window);
		draw_window_frame(&main_window, " NetDet ");
		draw_table_header(main_window.window);
		print_network_data(main_window.window);
		wrefresh(main_window.window);

		input = wgetch(main_window.window);
		input_handler(&main_window, input);
	}

	pthread_join(network_thread, NULL);
	pthread_join(signal_thread, NULL);
	close(shutdown_fd);

	delwin(main_window.window);
	endwin();

	get_error();
	return 0;
}
