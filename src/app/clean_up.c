#include "app/clean_up.h"
#include "app/lifecycle.h"
#include "common/error.h"
#include "ui/popup_templates.h"
#include "ui/tui.h"
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void main_clean_up(app_context *variables) {

	pthread_join(variables->network_thread, NULL);
	lifecycle_cleanup();

	for (uint32_t i = 0; i < variables->buffer.view.count; i++) {
		free(variables->buffer.data->items[i]);
		variables->buffer.data->items[i] = NULL;
	}

	free(variables->buffer.data->items);
	variables->buffer.data->items = NULL;

	free(variables->buffer.data);
	variables->buffer.data = NULL;

	close(variables->epoll_fd);

	if (variables->popup_window.panel != NULL) {
		del_panel(variables->popup_window.panel);
		delwin(variables->popup_window.window);
	}

	if (variables->popup_window.is_active) {
		popup_descriptor *descriptor = get_popup_descriptor(variables->popup_window.popup_type);

		if (descriptor->data_cleanup != NULL) {
			descriptor->data_cleanup(descriptor->args);
		}
	}

	del_panel(variables->main_window.panel);
	delwin(variables->main_window.window);
	endwin();

	get_error();

	return;
}

void popup_clean_up(popup_window_data *popup_window) {

	popup_window->is_active = false;
	popup_window->start_x = 0;
	popup_window->start_y = 0;
	popup_window->height = 0;
	popup_window->width = 0;

	del_panel(popup_window->panel);
	delwin(popup_window->window);
	popup_window->panel = NULL;
	popup_window->window = NULL;

	return;
}
