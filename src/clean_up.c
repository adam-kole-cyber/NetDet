#include "clean_up.h"
#include "device.h"
#include "error.h"
#include "shared_state.h"
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void main_clean_up(app_context *variables) {

	pthread_join(variables->network_thread, NULL);
	pthread_join(variables->signal_thread, NULL);

	for (uint32_t i = 0; i < variables->buffer.count; i++) {
		free(variables->buffer.items[i]);
		variables->buffer.items[i] = NULL;
	}

	free(variables->buffer.items);
	variables->buffer.items = NULL;

	close(variables->epoll_fd);
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	close(shutdown_main_fd);

	if (variables->popup_window.panel != NULL) {
		del_panel(variables->popup_window.panel);
		delwin(variables->popup_window.window);
	}
	if (interfaces != NULL) {
		for (int32_t i = 0; interfaces[i].if_name != NULL; i++) {
			free(interfaces[i].if_name);
		}
		free(interfaces);
	}

	del_panel(variables->main_window.panel);
	delwin(variables->main_window.window);
	endwin();

	get_error();

	return;
}

void network_clean_up(hash_map *map, int32_t *socket_fd, int32_t *epoll_fd) {

	free(map->table);
	map->table = NULL;

	pthread_mutex_lock(&binded_interface_mutex);
	free(binded_interface.if_name);
	pthread_mutex_unlock(&binded_interface_mutex);

	pthread_mutex_destroy(&binded_interface_mutex);

	close(bind_update_fd);
	close(*epoll_fd);
	close(*socket_fd);
	close(shutdown_network_fd);
}

void popup_clean_up(window_data *popup_window) {
	for (int32_t i = 0; interfaces[i].if_name != NULL; i++) {
		free(interfaces[i].if_name);
		interfaces[i].if_name = NULL;
	}
	free(interfaces);
	interfaces = NULL;

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
