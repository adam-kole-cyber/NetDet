#include "app_context.h"
#include "clean_up.h"
#include "device.h"
#include "error.h"
#include "init.h"
#include "shared_state.h"
#include "signal_handler.h"
#include "tui.h"
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>

atomic_bool end_main_loop = false;
atomic_uint_fast32_t termination_reason = PROGRAM_RUNNING;
int32_t shutdown_main_fd;
int32_t pipe_fd[2];
pthread_t signal_thread;

int main(int argc, char *argv[]) {
	app_context variables = {0};
	struct epoll_event events[3];
	struct network_thread_args args;

	args.argc = argc;
	args.argv = argv;

	ncurses_init();
	main_init(&variables, &args);

	draw(&variables);
	while (!atomic_load(&end_main_loop)) {
		int32_t number_of_events = epoll_wait(variables.epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == pipe_fd[0]) {
				ui_message msg;
				read(pipe_fd[0], &msg, sizeof(ui_message));

				if (msg.msg_type == UI_NEW_ENTRY) {
					if (slidingwindowbuffer_store_entry(&variables.buffer, msg.data) == -1) {
						main_error(APP_ERR_SLIDINGWINDOWBUFFER_STORE_ENTRY);
					}
				} else if (msg.msg_type == UI_RESIZE) {
					resize_handler(&variables);
				}

				draw(&variables);
			} else if (events[i].data.fd == STDIN_FILENO) {
				int32_t input = wgetch(variables.main_window.window);
				input_handler(input, &variables);

				draw(&variables);
			} else if (events[i].data.fd == shutdown_main_fd) {
				continue;
			}
		}
	}

	main_clean_up(&variables);
	return 0;
}
