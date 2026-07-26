#include "app_context.h"
#include "device.h"
#include "error.h"
#include "network.h"
#include "signal_handler.h"
#include "tui.h"
#include <bits/pthreadtypes.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>

atomic_bool end_main_loop = false;
atomic_uint_fast32_t termination_reason = PROGRAM_RUNNING;
int32_t shutdown_main_fd;
int32_t pipe_fd[2];
pthread_mutex_t device_data_structures_mutex;

int main(int argc, char *argv[]) {
	sliding_window_buffer buffer;
	int32_t epoll_fd;
	struct epoll_event register_event;
	struct epoll_event events[3];
	sigset_t mask;
	pthread_t signal_thread;
	pthread_t network_thread;
	struct network_thread_args args;
	window_data main_window;
	int32_t input = 0;

	app_context variables;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	shutdown_main_fd = eventfd(0, 0);
	pipe(pipe_fd);

	epoll_fd = epoll_create1(0);

	register_event.events = EPOLLIN;
	register_event.data.fd = shutdown_main_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, shutdown_main_fd, &register_event);

	register_event.events = EPOLLIN;
	register_event.data.fd = pipe_fd[0]; // reading end of the pipe
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd[0], &register_event);

	buffer.size = BUFFER_INITIAL_SIZE;
	buffer.items = calloc(buffer.size, sizeof(device *));
	buffer.count = 0;
	buffer.display_row = 0;
	buffer.display_limit = 0;
	buffer.head = 0;

	pthread_mutex_init(&device_data_structures_mutex, NULL);

	pthread_create(&signal_thread, NULL, signal_routine, NULL);

	args.argc = argc;
	args.argv = argv;
	args.signal_thread = signal_thread;
	pthread_create(&network_thread, NULL, network_routine, (void *)&args);

	ncurses_init();

	main_window.start_x = WINDOW_OUTER_INDENT;
	main_window.start_y = WINDOW_OUTER_INDENT;
	main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	main_window.window = newwin(main_window.height, main_window.width, main_window.start_y, main_window.start_x);
	keypad(main_window.window, TRUE);

	register_event.events = EPOLLIN;
	register_event.data.fd = STDIN_FILENO;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &register_event);

	pthread_mutex_lock(&device_data_structures_mutex);
	buffer.display_row =
		(main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) < 0 ? 0 : (main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;
	pthread_mutex_unlock(&device_data_structures_mutex);

	draw(&main_window, &buffer);
	while (!atomic_load(&end_main_loop)) {
		int32_t number_of_events = epoll_wait(epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == pipe_fd[0]) {
				ui_message msg;
				read(pipe_fd[0], &msg, sizeof(ui_message));

				if (msg.msg_type == UI_NEW_ENTRY) {
					slidingwindowbuffer_store_entry(&buffer, msg.data);
				} else if (msg.msg_type == UI_RESIZE) {
					struct winsize ws;
					ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
					resizeterm(ws.ws_row, ws.ws_col); // toto aktualizuje LINES/COLS

					main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
					main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
					wresize(main_window.window, main_window.height, main_window.width);
					mvwin(main_window.window, main_window.start_y, main_window.start_x);

					pthread_mutex_lock(&device_data_structures_mutex);
					buffer.display_row =
						(main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) < 0 ? 0 : (main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;
					pthread_mutex_unlock(&device_data_structures_mutex);
				}

				draw(&main_window, &buffer);
			} else if (events[i].data.fd == STDIN_FILENO) {
				input = wgetch(main_window.window);
				input_handler(input, &buffer);

				draw(&main_window, &buffer);
			} else if (events[i].data.fd == shutdown_main_fd) {
				continue;
			}
		}
	}

	pthread_join(network_thread, NULL);
	pthread_join(signal_thread, NULL);

	pthread_mutex_destroy(&device_data_structures_mutex);

	for (uint32_t i = 0; i < buffer.count; i++) {
		free(buffer.items[i]);
		buffer.items[i] = NULL;
	}

	free(buffer.items);
	buffer.items = NULL;

	close(epoll_fd);
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	close(shutdown_main_fd);
	delwin(main_window.window);
	endwin();

	get_error();
	return 0;
}
