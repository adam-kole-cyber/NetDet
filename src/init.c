#include "init.h"
#include "app_context.h"
#include "device.h"
#include "error.h"
#include "network.h"
#include "shared_state.h"
#include "signal_handler.h"
#include "tui.h"
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <locale.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <panel.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

void epoll_register(int32_t *epoll_fd, int32_t fd_to_register) {
	struct epoll_event register_event;

	register_event.events = EPOLLIN;
	register_event.data.fd = fd_to_register;
	epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd_to_register, &register_event);

	return;
}

void main_init(app_context *variables, struct network_thread_args *args) {
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	pthread_create(&variables->signal_thread, NULL, signal_routine, NULL);
	args->signal_thread = variables->signal_thread;
	pthread_create(&variables->network_thread, NULL, network_routine, (void *)args);

	pipe(pipe_fd);

	shutdown_main_fd = eventfd(0, 0);

	variables->epoll_fd = epoll_create1(0);
	epoll_register(&variables->epoll_fd, shutdown_main_fd);
	epoll_register(&variables->epoll_fd, pipe_fd[0]);
	epoll_register(&variables->epoll_fd, STDIN_FILENO);

	variables->main_window.is_active = true;
	variables->main_window.start_x = WINDOW_OUTER_INDENT;
	variables->main_window.start_y = WINDOW_OUTER_INDENT;
	variables->main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.window =
		newwin(variables->main_window.height, variables->main_window.width, variables->main_window.start_y, variables->main_window.start_x);
	variables->main_window.panel = new_panel(variables->main_window.window);
	keypad(variables->main_window.window, TRUE);

	variables->popup_window.is_active = false;
	variables->popup_window.start_x = 0;
	variables->popup_window.start_y = 0;
	variables->popup_window.width = 0;
	variables->popup_window.height = 0;
	variables->popup_window.window = NULL;
	variables->popup_window.panel = NULL;

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

void network_init(int32_t *socket_fd, struct network_thread_args *args, hash_map *map, int32_t *epoll_fd) {
	shutdown_network_fd = eventfd(0, 0);

	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		pthread_kill(args->signal_thread, SIGUSR1);
		pthread_exit(NULL);
		return;
	}

	if (args->argc > 1) {
		struct sockaddr_ll sll;
		memset(&sll, 0, sizeof(sll));
		sll.sll_family = AF_PACKET;
		sll.sll_protocol = htons(ETH_P_ALL);
		sll.sll_ifindex = if_nametoindex(args->argv[1]);

		if (sll.sll_ifindex == 0) {
			network_error(APP_ERR_IF_NAMETOINDEX, socket_fd, args->signal_thread);
			return;
		}

		if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
			network_error(APP_ERR_BIND, socket_fd, args->signal_thread);
			return;
		}

		binded_interface.if_index = sll.sll_ifindex;
		binded_interface.if_name = args->argv[1];
	}

	map->size = BUFFER_INITIAL_SIZE;
	map->count = 0;
	map->table = calloc(map->size, sizeof(hash_entry));

	*epoll_fd = epoll_create1(0);
	epoll_register(epoll_fd, shutdown_network_fd);
	epoll_register(epoll_fd, *socket_fd);
	return;
}

void popup_init(window_data *popup_window, const window_data *main_window, pthread_t signal_thread) {
	struct if_nameindex *kernel_interface = {0};
	int32_t count = 0;
	int32_t i = 0;

	popup_window->is_active = true;
	popup_window->start_x = main_window->start_x + 5;
	popup_window->start_y = main_window->start_y + 5;
	popup_window->height = main_window->height - 10;
	popup_window->width = main_window->width - 10;
	popup_window->window = newwin(popup_window->height, popup_window->width, popup_window->start_y, popup_window->start_x);
	popup_window->panel = new_panel(popup_window->window);

	kernel_interface = if_nameindex();
	if (kernel_interface == NULL) {
		main_error(APP_ERR_IF_NAMEINDEX, signal_thread);
		return;
	}

	while (kernel_interface[count].if_index != 0) {
		// is used to determine how many records an array contains
		count++;
	}

	interfaces = calloc(count + 2, sizeof(struct if_nameindex));
	if (interfaces == NULL) {
		main_error(APP_ERR_IF_NAMEINDEX, signal_thread);
		return;
	}

	for (i = 0; i < count; i++) {
		interfaces[i].if_index = kernel_interface[i].if_index;
		interfaces[i].if_name = strdup(kernel_interface[i].if_name);
	}
	if_freenameindex(kernel_interface);

	interfaces[count].if_index = 0;
	interfaces[count].if_name = strdup("all");

	return;
}

void ncurses_init(void) {
	setlocale(LC_ALL, "");
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(0);

	if (has_colors()) { // enables colors in terminal
		start_color();
		use_default_colors();
		init_pair(1, COLOR_GREEN, -1);
		init_pair(2, COLOR_YELLOW, -1);
		init_pair(3, -1, COLOR_BLACK);
		init_pair(4, COLOR_RED, -1);
		init_pair(5, COLOR_CYAN, -1);
		init_pair(6, COLOR_RED, COLOR_BLACK);
		init_pair(7, COLOR_YELLOW, COLOR_BLACK);
		init_pair(8, COLOR_GREEN, COLOR_BLACK);
	}

	return;
}
