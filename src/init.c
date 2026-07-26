#include "init.h"
#include "app_context.h"
#include "network.h"
#include "shared_state.h"
#include "signal_handler.h"
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

static void epoll_register(int32_t *epoll_fd, int32_t fd_to_register) {
	struct epoll_event register_event;

	register_event.events = EPOLLIN;
	register_event.data.fd = fd_to_register;
	epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd_to_register, &register_event);

	return;
}

void main_init(app_context *variables, int argc, char *argv[]) {
	sigset_t mask;
	struct network_thread_args args;

	args.argc = argc;
	args.argv = argv;
	args.signal_thread = variables->signal_thread;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	pthread_mutex_init(&device_data_structures_mutex, NULL);
	pthread_create(&variables->signal_thread, NULL, signal_routine, NULL);
	pthread_create(&variables->network_thread, NULL, network_routine, (void *)&args);

	pipe(pipe_fd);

	shutdown_main_fd = eventfd(0, 0);

	variables->epoll_fd = epoll_create1(0);
	epoll_register(&variables->epoll_fd, shutdown_main_fd);
	epoll_register(&variables->epoll_fd, pipe_fd[0]);
	epoll_register(&variables->epoll_fd, STDIN_FILENO);

	variables->main_window.start_x = WINDOW_OUTER_INDENT;
	variables->main_window.start_y = WINDOW_OUTER_INDENT;
	variables->main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.window =
		newwin(variables->main_window.height, variables->main_window.width, variables->main_window.start_y, variables->main_window.start_x);
	keypad(variables->main_window.window, TRUE);

	uint32_t i = (variables->main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) < 0
					 ? 0
					 : (variables->main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;

	variables->buffer.size = BUFFER_INITIAL_SIZE;
	variables->buffer.items = calloc(variables->buffer.size, sizeof(device *));
	variables->buffer.count = 0;
	variables->buffer.display_limit = 0;
	variables->buffer.head = 0;
	atomic_store(&variables->buffer.display_row, i);

	return;
}
