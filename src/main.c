#include "app_context.h"
#include "device.h"
#include "error.h"
#include "init.h"
#include "signal_handler.h"
#include "tui.h"
#include <ncurses.h>
#include <pthread.h>
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
	app_context variables;
	struct epoll_event events[3];

	ncurses_init();
	main_init(&variables, argc, argv);

	draw(&variables.main_window, &variables.buffer);
	while (!atomic_load(&end_main_loop)) {
		int32_t number_of_events = epoll_wait(variables.epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == pipe_fd[0]) {
				ui_message msg;
				read(pipe_fd[0], &msg, sizeof(ui_message));

				if (msg.msg_type == UI_NEW_ENTRY) {
					slidingwindowbuffer_store_entry(&variables.buffer, msg.data);
				} else if (msg.msg_type == UI_RESIZE) {
					struct winsize ws;
					ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
					resizeterm(ws.ws_row, ws.ws_col); // toto aktualizuje LINES/COLS

					variables.main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
					variables.main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
					wresize(variables.main_window.window, variables.main_window.height, variables.main_window.width);
					mvwin(variables.main_window.window, variables.main_window.start_y, variables.main_window.start_x);

					uint32_t i = (variables.main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) < 0
									 ? 0
									 : (variables.main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;

					atomic_store(&variables.buffer.display_row, i);
				}

				draw(&variables.main_window, &variables.buffer);
			} else if (events[i].data.fd == STDIN_FILENO) {
				int32_t input = wgetch(variables.main_window.window);
				input_handler(input, &variables.buffer);

				draw(&variables.main_window, &variables.buffer);
			} else if (events[i].data.fd == shutdown_main_fd) {
				continue;
			}
		}
	}

	pthread_join(variables.network_thread, NULL);
	pthread_join(variables.signal_thread, NULL);

	pthread_mutex_destroy(&device_data_structures_mutex);

	for (uint32_t i = 0; i < variables.buffer.count; i++) {
		free(variables.buffer.items[i]);
		variables.buffer.items[i] = NULL;
	}

	free(variables.buffer.items);
	variables.buffer.items = NULL;

	close(variables.epoll_fd);
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	close(shutdown_main_fd);
	delwin(variables.main_window.window);
	endwin();

	get_error();
	return 0;
}
