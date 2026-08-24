#include "ui/popup_inspect.h"
#include "core/device.h"
#include <math.h>
#include <ncurses.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t *ip;
static uint32_t *qinq_tag;
static uint32_t *dot1q_tag;
static atomic_uint_least64_t *atomic_int_storage;
static rate_history *rate;
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

static uint64_t get_atomic_int64(void *arg) {
	atomic_uint_least64_t *atomic_int_storage = *(atomic_uint_least64_t **)arg;
	return atomic_load(atomic_int_storage);
}

static uint32_t get_rate(void *arg) {
	rate_history *graph = *(rate_history **)arg;
	return atomic_load(&graph->data[graph->head - 1]);
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
		uint32_t block_index =
			(int)round((double)atomic_load(&graph->data[(graph->head + i) % RATE_HISTORY_SIZE]) / max * 7.0);
		strcpy(buffer[i], blocks[block_index]);
	}

	for (uint32_t i = 0; i < RATE_HISTORY_SIZE; i++) {
		strcat(result, buffer[i]);
	}

	return result;
}

const inspect_field_t popup_inspect[INSPECT_FIELD_COUNT] = {
	{"IP: %s", IP, .getter = {.get_ip = get_ip}, .arg = &ip},
	{"802.1ad (QinQ): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter = {.get_vlan_tag = get_vlan_tag}, .arg = &qinq_tag},
	{"802.1Q (Dot1Q): %d\tCoS: %d\tDEI: %d", VLAN_TAG, .getter = {.get_vlan_tag = get_vlan_tag}, .arg = &dot1q_tag},
	{"Total frames: %d", ATOMIC_INT, .getter = {.get_atomic_int64 = get_atomic_int64}, .arg = &atomic_int_storage},
	{"Rate history: %u/s", RATE, .getter = {.get_rate = get_rate}, .arg = &rate},
	{"%s", GRAPH, .getter = {.get_graph = get_graph}, .arg = &graph}};

void prepare_inspect_data(void *args) {
	device *action_device = (device *)args;

	ip = action_device->ip;
	qinq_tag = &action_device->qinq_tag;
	dot1q_tag = &action_device->dot1q_tag;
	atomic_int_storage = &action_device->previous_frames;
	rate = &action_device->graph;
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
			mvwprintw(popup_window, row, col, items[index].string, (vlan_tag & 0xfff), (vlan_tag & 0xe000) >> 13,
					  (vlan_tag & 0x1000) >> 12);
			break;
		}
		case ATOMIC_INT:
			mvwprintw(popup_window, row, col, items[index].string,
					  items[index].getter.get_atomic_int64(items[index].arg));
			break;
		case GRAPH:
			mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_graph(items[index].arg));
			break;
		case RATE:
			mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_rate(items[index].arg));
			break;
		default:
			break;
	}

	return;
}
