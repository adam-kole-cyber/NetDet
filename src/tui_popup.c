#include "tui_popup.h"
#include "clean_up.h"
#include "device.h"
#include "error.h"
#include "init.h"
#include "scroll_view.h"
#include "shared_state.h"
#include "tui.h"
#include <math.h>
#include <ncurses.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *ip;
static uint32_t *qinq_tag;
static uint32_t *dot1q_tag;
static atomic_uint_least64_t *atomic_int_storage;
static rate_history *graph;

static const char *get_ip(void *arg) {
	uint8_t *ip = *(uint8_t **)arg;
	static char ip_buffer[16];
	snprintf(ip_buffer, sizeof(ip_buffer), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return ip_buffer;
}

static uint32_t get_vlan_tag(void *arg) {
	uint32_t *vlan_tag = *(uint32_t **)arg;
	return *vlan_tag;
}

static uint32_t get_atomic_int(void *arg) {
	atomic_uint_least64_t *atomic_int_storage = *(atomic_uint_least64_t **)arg;
	return atomic_load(atomic_int_storage);
}

static const char *get_graph(void *arg) {
	rate_history *graph = *(rate_history **)arg;
	static char buffer[RATE_HISTORY_SIZE][4];
	static char result[RATE_HISTORY_SIZE * 3 + 1] = {0};
	result[0] = '\0';
	const char *blocks[] = {
		"\u2581", // ▁
		"\u2582", // ▂
		"\u2583", // ▃
		"\u2584", // ▄
		"\u2585", // ▅
		"\u2586", // ▆
		"\u2587", // ▇
		"\u2588", // █
	};

	uint32_t max = atomic_load(&graph->data[0]);
	for (uint32_t i = 0; i < RATE_HISTORY_SIZE; i++) {
		uint32_t current_value = atomic_load(&graph->data[i]);
		if (current_value > max) {
			max = current_value;
		}
	}

	if (max == 0) {
		max = 1;
	}

	for (uint32_t i = 0; i < RATE_HISTORY_SIZE; i++) {
		uint32_t block_index = (int)round((double)atomic_load(&graph->data[(graph->head + i) % RATE_HISTORY_SIZE]) / max * 7.0);
		strcpy(buffer[i], blocks[block_index]);
	}

	for (uint32_t i = 0; i < RATE_HISTORY_SIZE; i++) {
		strcat(result, buffer[i]);
	}

	return result;
}

interfaces_list popup_list = {0};
const inspect_field_t popup_inspect[] = {{"IP: %s", IP, .getter.get_ip = get_ip, .arg = &ip},
										 {"802.1ad (QinQ): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter.get_vlan_tag = get_vlan_tag, .arg = &qinq_tag},
										 {"802.1Q (Dot1Q): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter.get_vlan_tag = get_vlan_tag, .arg = &dot1q_tag},
										 {"Total frames: %d", ATOMIC_INT, .getter.get_atomic_int = get_atomic_int, .arg = &atomic_int_storage},
										 {"Rate history: %s", GRAPH, .getter.get_graph = get_graph, .arg = &graph}};
scroll_view inspect_field;

static void prepare_interface_list(pthread_t signal_thread) {
	struct if_nameindex *kernel_interface = NULL;
	int32_t count = 0;
	int32_t i = 0;

	kernel_interface = if_nameindex();
	if (kernel_interface == NULL) {
		main_error(APP_ERR_IF_NAMEINDEX, signal_thread);
		return;
	}

	while (kernel_interface[count].if_index != 0) {
		// is used to determine how many records an array contains
		count++;
	}

	popup_list.items = calloc(count + 2, sizeof(struct if_nameindex));
	if (popup_list.items == NULL) {
		main_error(APP_ERR_IF_NAMEINDEX, signal_thread);
		return;
	}

	for (i = 0; i < count; i++) {
		popup_list.items[i].if_index = kernel_interface[i].if_index;
		popup_list.items[i].if_name = strdup(kernel_interface[i].if_name);
	}
	if_freenameindex(kernel_interface);

	popup_list.items[count].if_index = UINT32_MAX;
	popup_list.items[count].if_name = strdup("all");

	return;
}

static void prepare_scroll_view(popup_window_data *popup_window) {
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

	return;
}

static void interfaces_list_clean_up(void) {
	for (int32_t i = 0; popup_list.items[i].if_name != NULL; i++) {
		free(popup_list.items[i].if_name);
		popup_list.items[i].if_name = NULL;
	}
	free(popup_list.items);
	popup_list.items = NULL;

	return;
}

void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_type, device *action_device,
						 pthread_t signal_thread) {
	static bool is_visible = false;

	is_visible = !is_visible;

	if (is_visible) {
		main_window->is_active = false;

		popup_init(popup_window, main_window, window_type);

		switch (window_type) {
		case INTERFACES_LIST:
			prepare_interface_list(signal_thread);
			prepare_scroll_view(popup_window);
			draw_window_frame((window_data *)popup_window, " Available interfaces "); // TODO fix this properly
			draw_popup(popup_window);
			break;
		case INSPECT_LIST:
			ip = action_device->ip;
			qinq_tag = &action_device->qinq_tag;
			dot1q_tag = &action_device->dot1q_tag;
			atomic_int_storage = &action_device->previsou_frames;
			graph = &action_device->graph;

			inspect_field.count = sizeof(popup_inspect) / sizeof(popup_inspect[0]);
			inspect_field.visible = popup_window->height - 2;

			if (inspect_field.visible > inspect_field.count) {
				inspect_field.visible = inspect_field.count;
			}

			inspect_field.head = 0;
			inspect_field.cursor = 0;

			draw_window_frame((window_data *)popup_window, " Inspect ");
			draw_popup(popup_window);
			break;
		default:
			break;
		}

	} else {
		main_window->is_active = true;
		popup_clean_up(popup_window);

		switch (popup_window->popup_type) {
		case INTERFACES_LIST:
			interfaces_list_clean_up();
			break;
		case INSPECT_LIST:
			break;
		default:
			break;
		}
	}

	return;
}

void draw_popup(popup_window_data *popup_window) {
	if (popup_window->popup_type == INTERFACES_LIST) {

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
	} else {
		for (uint32_t i = 0; i < inspect_field.visible; i++) {
			int32_t index = inspect_field.head + i;

			if ((uint32_t)index >= inspect_field.count) {
				break;
			}

			if ((uint32_t)inspect_field.cursor == i) {
				wattron(popup_window->window, COLOR_PAIR(3));
			}

			switch (popup_inspect[i].getter_type) {
			case IP:
				mvwprintw(popup_window->window, 1 + i, 2, popup_inspect[index].string, popup_inspect[index].getter.get_ip(popup_inspect[index].arg));
				break;
			case VLAN_TAG: {
				uint32_t vlan_tag = popup_inspect[index].getter.get_vlan_tag(popup_inspect[index].arg);
				mvwprintw(popup_window->window, 1 + i, 2, popup_inspect[index].string, (vlan_tag & 0xfff), (vlan_tag & 0xe000) >> 13,
						  (vlan_tag & 0x1000) >> 12);
				break;
			}
			case ATOMIC_INT:
				mvwprintw(popup_window->window, 1 + i, 2, popup_inspect[index].string,
						  popup_inspect[index].getter.get_atomic_int(popup_inspect[index].arg));
				break;
			case GRAPH:
				mvwprintw(popup_window->window, 1 + i, 2, popup_inspect[index].string,
						  popup_inspect[index].getter.get_graph(popup_inspect[index].arg));
				break;
			default:
				break;
			}

			if ((uint32_t)inspect_field.cursor == i) {
				wattroff(popup_window->window, COLOR_PAIR(3));
			}
		}
	}
	return;
}
