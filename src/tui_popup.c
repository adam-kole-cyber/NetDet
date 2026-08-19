#include "tui_popup.h"
#include "clean_up.h"
#include "device.h"
#include "init.h"
#include "popup_templates.h"
#include "scroll_view.h"
#include "shared_state.h"
#include "tui.h"
#include <math.h>
#include <ncurses.h>
#include <net/if.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
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

const inspect_field_t popup_inspect[6] = {
	{"IP: %s", IP, .getter = {.get_ip = get_ip}, .arg = &ip},
	{"802.1ad (QinQ): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter = {.get_vlan_tag = get_vlan_tag}, .arg = &qinq_tag},
	{"802.1Q (Dot1Q): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter = {.get_vlan_tag = get_vlan_tag}, .arg = &dot1q_tag},
	{"Total frames: %d", ATOMIC_INT, .getter = {.get_atomic_int = get_atomic_int}, .arg = &atomic_int_storage},
	{"Rate history:", NONE, .getter = {.get_graph = get_graph}, .arg = &graph},
	{"%s", GRAPH, .getter = {.get_graph = get_graph}, .arg = &graph}};

void prepare_inspect_data(void *args) {
	device *action_device = (device *)args;

	ip = action_device->ip;
	qinq_tag = &action_device->qinq_tag;
	dot1q_tag = &action_device->dot1q_tag;
	atomic_int_storage = &action_device->previsou_frames;
	graph = &action_device->graph;

	return;
}

void render_inspect_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data) {
	const inspect_field_t *items = (const inspect_field_t *)data;

	switch (popup_inspect[index].getter_type) {
	case IP:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_ip(items[index].arg));
		break;
	case VLAN_TAG: {
		uint32_t vlan_tag = items[index].getter.get_vlan_tag(items[index].arg);
		mvwprintw(popup_window, row, col, items[index].string, (vlan_tag & 0xfff), (vlan_tag & 0xe000) >> 13, (vlan_tag & 0x1000) >> 12);
		break;
	}
	case ATOMIC_INT:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_atomic_int(items[index].arg));
		break;
	case GRAPH:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_graph(items[index].arg));
		break;
	case NONE:
		mvwprintw(popup_window, row, col, "%s", items[index].string);
		break;
	default:
		break;
	}

	return;
}

void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_type, device *action_device) {
	static bool is_visible = false;

	is_visible = !is_visible;

	if (is_visible) {
		main_window->is_active = false;

		popup_init(popup_window, main_window, window_type);

		popup_descriptor *descriptor = get_popup_descriptor(popup_window->popup_type);
		descriptor->args = action_device;
		descriptor->data_init(action_device);

		scroll_view_configure(&descriptor->view, descriptor->data_count, popup_window->height);
		descriptor->view.head = 0;
		descriptor->view.cursor = 0;

		draw_window_frame((window_data *)popup_window, get_popup_descriptor_title(popup_window->popup_type));
		draw_popup(popup_window);
	} else {
		main_window->is_active = true;
		popup_clean_up(popup_window);

		popup_descriptor *descriptor = get_popup_descriptor(popup_window->popup_type);

		if (descriptor->data_cleanup != NULL) {
			descriptor->data_cleanup(descriptor->args);
		}
	}

	return;
}
