#include "tui_app.h"
#include "device.h"
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>

static table_layout current_layout = {0};

static inline void sync_display_limit(device_table_view *buffer) {
	buffer->view.visible =
		(atomic_load(&buffer->view.viewport_capacity) < buffer->data->size) ? atomic_load(&buffer->view.viewport_capacity) : buffer->data->size;
}

static void print_mac(WINDOW *window, int32_t row, int32_t column, const uint8_t *mac, bool highlight_line) {
	short pair = 0;
	char mac_string[18];

	snprintf(mac_string, sizeof(mac_string), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if (highlight_line) {
		if ((mac[0] & 0x03) == 0x03)
			pair = 6;
		else if ((mac[0] & 0x02) == 0x02)
			pair = 7;
		else if ((mac[0] & 0x01) == 0x01)
			pair = 8;
		else
			pair = 3;
	} else {
		if ((mac[0] & 0x03) == 0x03)
			pair = 4;
		else if ((mac[0] & 0x02) == 0x02)
			pair = 2;
		else if ((mac[0] & 0x01) == 0x01)
			pair = 1;
		else
			pair = 0;
	}

	wattrset(window, pair ? COLOR_PAIR(pair) : A_NORMAL);
	mvwprintw(window, row, column, "%-*s", current_layout.col_width, mac_string);
	wattrset(window, A_NORMAL);

	if (highlight_line) {
		wattron(window, COLOR_PAIR(3));
	}

	return;
}

static void print_ip(WINDOW *window, const uint8_t *ip) {
	char ip_string[16];

	snprintf(ip_string, sizeof(ip_string), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
	wprintw(window, "%-*s", current_layout.col_width, ip_string);
	return;
}

static void print_qinq(WINDOW *window, const uint32_t *qinq_tag, bool highlight_line) {
	if ((*qinq_tag & 0xffff) != 0) {
		char qinq_string[5];

		snprintf(qinq_string, sizeof(qinq_string), "%u", (*qinq_tag & 0xfff));

		if ((*qinq_tag & 0xf000) != 0) {
			int32_t pair = 0;

			if (highlight_line) {
				pair = 9;
			} else {
				pair = 5;
			}

			wattrset(window, COLOR_PAIR(pair));
		}

		wprintw(window, "%-*s", current_layout.col_width, qinq_string);
		wattrset(window, A_NORMAL);

		if (highlight_line) {
			wattron(window, COLOR_PAIR(3));
		}

	} else {
		int32_t pair = 0;
		if (highlight_line) {
			pair = 6;
		} else {
			pair = 4;
		}

		wattrset(window, COLOR_PAIR(pair));
		wprintw(window, "%-*s", current_layout.col_width, "-");
		wattrset(window, A_NORMAL);

		if (highlight_line) {
			wattron(window, COLOR_PAIR(3));
		}
	}
	return;
}

static void print_dot1q(WINDOW *window, const uint32_t *dot1q_tag, bool highlight_line) {
	if ((*dot1q_tag & 0xffff) != 0) {
		char dot1q_string[5];

		snprintf(dot1q_string, sizeof(dot1q_string), "%u", (*dot1q_tag & 0xfff));

		if ((*dot1q_tag & 0xf000) != 0) {
			int32_t pair = 0;

			if (highlight_line) {
				pair = 9;
			} else {
				pair = 5;
			}

			wattrset(window, COLOR_PAIR(pair));
		}

		wprintw(window, "%-*s", (current_layout.col_width / 2), dot1q_string);
		wattrset(window, A_NORMAL);

		if (highlight_line) {
			wattron(window, COLOR_PAIR(3));
		}
	} else {
		int32_t pair = 0;
		if (highlight_line) {
			pair = 6;
		} else {
			pair = 4;
		}

		wattrset(window, COLOR_PAIR(pair));
		wprintw(window, "%-*s", (current_layout.col_width / 2), "-");
		wattrset(window, A_NORMAL);

		if (highlight_line) {
			wattron(window, COLOR_PAIR(3));
		}
	}
	return;
}

static void print_lastseen(WINDOW *window, const time_struct *last_seen) {
	char last_seen_string[9];

	snprintf(last_seen_string, sizeof(last_seen_string), "%02u:%02u:%02u", atomic_load(&last_seen->hour), atomic_load(&last_seen->minutes),
			 atomic_load(&last_seen->seconds));
	wprintw(window, "%*s", ((current_layout.col_width / 2) + current_layout.col_width_remainder), last_seen_string);
	return;
}

static void print_network_row(WINDOW *window, int32_t row, int32_t column, const device *device_data, bool highlight_line) {
	print_mac(window, row, column, device_data->mac, highlight_line);
	print_ip(window, device_data->ip);
	print_qinq(window, &device_data->qinq_tag, highlight_line);
	print_dot1q(window, &device_data->dot1q_tag, highlight_line);
	print_lastseen(window, &device_data->last_seen);

	return;
}

void draw_table_header(WINDOW *window) {
	wattron(window, COLOR_PAIR(2));

	mvwprintw(window, 1, 2, "%-*s%-*s%-*s%-*s%*s", current_layout.col_width, "MAC", current_layout.col_width, "IP", current_layout.col_width,
			  "802.1ad", (current_layout.col_width / 2), "802.1Q", ((current_layout.col_width / 2) + current_layout.col_width_remainder),
			  "Last seen");
	wattroff(window, COLOR_PAIR(2));

	return;
}

void print_network_data(WINDOW *window, device_table_view *buffer) {
	int32_t display_row_start = 2;

	sync_display_limit(buffer);

	uint32_t limit = buffer->view.visible;
	if (buffer->view.head + limit > buffer->view.count) {
		limit = buffer->view.count - buffer->view.head;
	}

	for (uint32_t i = 0; i < limit; i++) {
		if (i == (uint32_t)buffer->view.cursor) {
			wattron(window, COLOR_PAIR(3));
		}

		print_network_row(window, display_row_start + i, 2, buffer->data->items[buffer->view.head + i], i == (uint32_t)buffer->view.cursor);

		if (i == (uint32_t)buffer->view.cursor) {
			wattroff(window, COLOR_PAIR(3));
		}
	}
	return;
}

static table_layout table_layout_compute(int32_t window_width) {
	table_layout layout = {.col_width = (window_width - 4) / 4, .col_width_remainder = (window_width - 4) % 4};
	return layout;
}

void set_column_width(int32_t window_width) {
	current_layout = table_layout_compute(window_width);
	return;
}
