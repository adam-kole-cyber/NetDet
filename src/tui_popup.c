#include "tui_popup.h"
#include "clean_up.h"
#include "init.h"
#include "shared_state.h"
#include "tui.h"
#include <stdint.h>
#include <stdio.h>

static uint32_t *ip;

static const char *get_string(void) { return "hello"; }

static const char *get_ip(void *arg) {
	uint32_t *ip = (uint32_t *)arg;
	static char ip_buffer[16];
	snprintf(ip_buffer, sizeof(ip_buffer), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return ip_buffer;
}

static int get_int(void) { return 107; }

interfaces_list popup_list = {0};
const inspect_field_t popup_inspect[] = {{"IP: %d", .getter.get_ip = get_ip, .arg = &ip},
										 {"802.1ad (QinQ): %d\tCoS: %d\tDEI: %d", .getter.get_int = get_int},
										 {"802.1Q (Dot1Q): %d\tCoS: %d\tDEI: %d", .getter.get_int = get_int},
										 {"Total frames: %d", .getter.get_int = get_int},
										 {"Rate history: %s", .getter.get_str = get_string}};

void popup_window_action(window_data *main_window, window_data *popup_window, pthread_t signal_thread) {
	static bool is_visible = false;

	is_visible = !is_visible;

	if (is_visible) {
		main_window->is_active = false;

		popup_init(popup_window, main_window, signal_thread);

		popup_list.view.count = 0;
		while (popup_list.items[popup_list.view.count].if_index != 0) { // this is partiali universal
			popup_list.view.count++;
		}

		popup_list.view.visible = popup_window->height - 2;

		if (popup_list.view.visible > popup_list.view.count) {
			popup_list.view.visible = popup_list.view.count;
		}

		popup_list.view.head = 0;
		popup_list.view.cursor = 0;

		draw_window_frame(popup_window, " Available interfaces ");
		draw_popup(popup_window);
	} else {
		main_window->is_active = true;
		popup_clean_up(popup_window);
	}

	return;
}

void draw_popup(window_data *popup_window) {
	for (uint32_t i = 0; i < popup_list.view.visible; i++) {
		int32_t index = popup_list.view.head + i;

		if ((uint32_t)index >= popup_list.view.count) {
			break;
		}

		if ((uint32_t)popup_list.view.cursor == i) {
			wattron(popup_window->window, COLOR_PAIR(3));
		}

		pthread_mutex_lock(&binded_interface_mutex);
		mvwprintw(popup_window->window, 1 + i, 2, "[%c] - %s", (binded_interface.if_index == popup_list.items[index].if_index) ? '*' : ' ',
				  popup_list.items[index].if_name);
		pthread_mutex_unlock(&binded_interface_mutex);

		if ((uint32_t)popup_list.view.cursor == i) {
			wattroff(popup_window->window, COLOR_PAIR(3));
		}
	}

	return;
}
