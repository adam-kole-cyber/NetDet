#include "tui_popup.h"
#include "clean_up.h"
#include "device.h"
#include "init.h"
#include "shared_state.h"
#include "tui.h"
#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t *ip;
static uint32_t *vlan_tag;
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
const inspect_field_t popup_inspect[] = {{"IP: %s", .getter.get_ip = get_ip, .arg = &ip},
										 {"802.1ad (QinQ): %d\tCoS: %d\tDEI: %d", .getter.get_vlan_tag = get_vlan_tag, .arg = &vlan_tag},
										 {"802.1Q (Dot1Q): %d\tCoS: %d\tDEI: %d", .getter.get_vlan_tag = get_vlan_tag, .arg = &vlan_tag},
										 {"Total frames: %d", .getter.get_atomic_int = get_atomic_int, .arg = &atomic_int_storage},
										 {"Rate history: %s", .getter.get_graph = get_graph, .arg = &graph}};

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
