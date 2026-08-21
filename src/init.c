#include "init.h"
#include "device.h"
#include "epoll_utils.h"
#include "error.h"
#include "lifecycle.h"
#include "net/network_thread.h"
#include "tui.h"
#include "tui_app.h"
#include "ui/popup_templates.h"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <locale.h>
#include <ncurses.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <panel.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

void main_init(app_context *variables, struct network_thread_args *args) {
	sigset_t mask;

	variables->epoll_fd = epoll_create1(0);
	lifecycle_init(variables->epoll_fd);
	epoll_register(variables->epoll_fd, STDIN_FILENO);

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	pthread_create(&variables->network_thread, NULL, network_routine, (void *)args);

	variables->main_window.is_active = true;
	variables->main_window.start_x = WINDOW_OUTER_INDENT;
	variables->main_window.start_y = WINDOW_OUTER_INDENT;
	variables->main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.window =
		newwin(variables->main_window.height, variables->main_window.width, variables->main_window.start_y, variables->main_window.start_x);
	variables->main_window.panel = new_panel(variables->main_window.window);
	keypad(variables->main_window.window, TRUE);

	set_column_width(variables->main_window.width);

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
	if (variables->buffer.items == NULL) {
		main_error(APP_ERR_CALLOC);
		return;
	}
	variables->buffer.view.count = 0;
	variables->buffer.view.visible = 0;
	variables->buffer.view.head = 0;
	variables->buffer.view.cursor = 0;
	atomic_store(&variables->buffer.view.viewport_capacity, i);

	return;
}

void popup_init(popup_window_data *popup_window, const window_data *main_window, popup_type window_type) {
	popup_window->is_active = true;
	popup_window->popup_type = window_type;

	if ((main_window->height - 10) >= 3) {
		popup_window->start_x = main_window->start_x + 5;
		popup_window->start_y = main_window->start_y + 5;
		popup_window->width = main_window->width - 10;
		popup_window->height = main_window->height - 10;
	} else {
		popup_window->start_x = main_window->start_x;
		popup_window->start_y = main_window->start_y;
		popup_window->width = main_window->width;
		popup_window->height = main_window->height;
	}

	popup_window->window = newwin(popup_window->height, popup_window->width, popup_window->start_y, popup_window->start_x);
	popup_window->panel = new_panel(popup_window->window);

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
		init_pair(9, COLOR_CYAN, COLOR_BLACK);
	}

	return;
}

void signal_init(sigset_t *mask) {
	sigemptyset(mask);
	sigaddset(mask, SIGINT);
	sigaddset(mask, SIGWINCH);
	sigaddset(mask, SIGUSR1);

	return;
}
