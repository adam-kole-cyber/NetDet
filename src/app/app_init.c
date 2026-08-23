#include "app/app_init.h"
#include "app/lifecycle.h"
#include "common/error.h"
#include "core/device.h"
#include "net/network_thread.h"
#include "platform/epoll_utils.h"
#include "ui/popup_templates.h"
#include "ui/tui.h"
#include "ui/tui_app.h"
#include <bits/types/sigset_t.h>
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
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
	variables->main_window.window = newwin(variables->main_window.height, variables->main_window.width,
										   variables->main_window.start_y, variables->main_window.start_x);
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

	variables->buffer.data = calloc(1, sizeof(device_buffer));
	variables->buffer.data->size = BUFFER_INITIAL_SIZE;
	variables->buffer.data->items = calloc(variables->buffer.data->size, sizeof(device *));
	if (variables->buffer.data->items == NULL) {
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
