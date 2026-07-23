#include "tui.h"
#include "device.h"
#include "network.h"
#include <bits/pthreadtypes.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

static int32_t cursor_position = 0;

static void print_mac(WINDOW *window, int32_t row, int32_t column, const uint8_t *mac, bool highlight_line) {
	short pair = 0;

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
			pair = 5;
		else if ((mac[0] & 0x01) == 0x01)
			pair = 1;
		else
			pair = 0;
	}

	wattrset(window, pair ? COLOR_PAIR(pair) : A_NORMAL);
	mvwprintw(window, row, column, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	wattrset(window, A_NORMAL);

	if (highlight_line) {
		wattron(window, COLOR_PAIR(3));
	}

	return;
}
static void print_ip(WINDOW *window, const uint8_t *ip) {
	wprintw(window, "\t%d.%d.%d.%d\t", ip[0], ip[1], ip[2], ip[3]);
	return;
}

static void print_qinq(WINDOW *window, const uint32_t *qinq_tag) {
	wprintw(window, "%d\t\t", *qinq_tag);
	return;
}

static void print_dot1q(WINDOW *window, const uint32_t *dot1q_tag) {
	wprintw(window, "%d\t\t", *dot1q_tag);
	return;
}

static void print_lastseen(WINDOW *window, const time_struct *last_seen) {
	wprintw(window, "%02d:%02d:%02d", last_seen->hour, last_seen->minutes, last_seen->seconds);
	return;
}

static void print_network_row(WINDOW *window, int32_t row, int32_t column, const device *device_data, bool highlight_line) {
	print_mac(window, row, column, device_data->mac, highlight_line);
	print_ip(window, device_data->ip);
	print_qinq(window, &device_data->qinq_tag);
	print_dot1q(window, &device_data->dot1q_tag);
	print_lastseen(window, &device_data->last_seen);

	return;
}

static inline void sync_display_limit(void) { buffer.display_limit = (buffer.display_row < buffer.size) ? buffer.display_row : buffer.size; }

static void resize_handler(window_data *window_data) {
	int32_t new_height = LINES - (WINDOW_OUTER_INDENT * 2);
	int32_t new_width = COLS - (WINDOW_OUTER_INDENT * 2);

	for (int32_t i = 0; i < window_data->height; i++) { // cleaning leftovers from old window
		for (int32_t j = 0; j < window_data->width; j++) {
			mvwprintw(window_data->window, i, j, " ");
		}
	}

	window_data->height = new_height;
	window_data->width = new_width;

	int32_t computed_limit = (window_data->height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;

	pthread_mutex_lock(&device_data_structures_mutex);
	buffer.display_row = (computed_limit) < 0 ? 0 : computed_limit;
	pthread_mutex_unlock(&device_data_structures_mutex);

	wnoutrefresh(window_data->window);
	doupdate();

	wresize(window_data->window, new_height, new_width);
	mvwin(window_data->window, WINDOW_OUTER_INDENT, WINDOW_OUTER_INDENT);

	return;
}

static void cursor_move(int32_t direction) {
	int32_t new_position = cursor_position + direction;

	pthread_mutex_lock(&device_data_structures_mutex);
	sync_display_limit();

	if (new_position < 0) {
		if (buffer.head > 0) {
			buffer.head--;
		}
		cursor_position = 0;
	} else if ((uint32_t)new_position >= buffer.display_limit && (buffer.head + buffer.display_limit) < buffer.count) {
		buffer.head++;
		cursor_position = buffer.display_limit - 1;
	} else {
		if (buffer.head + (uint32_t)new_position >= buffer.count) {
			pthread_mutex_unlock(&device_data_structures_mutex);
			return;
		}
		cursor_position = new_position;
	}
	pthread_mutex_unlock(&device_data_structures_mutex);

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
		init_pair(7, COLOR_CYAN, COLOR_BLACK);
		init_pair(8, COLOR_GREEN, COLOR_BLACK);
	}

	return;
}

void draw_window_frame(window_data *window_data, const char *title) {
	bool title_set = title != NULL;
	int32_t last_usable_column = window_data->width - 1; // the window width will be reduced due to the frame
	int32_t last_usable_row = window_data->height - 1;
	int32_t minimum_title_area = last_usable_column - 2; // to ensure that title is not sticked together with corners

	mvwprintw(window_data->window, 0, 0, "╭"); // draw the corners of the frame
	mvwprintw(window_data->window, 0, last_usable_column, "╮");
	mvwprintw(window_data->window, last_usable_row, 0, "╰");
	mvwprintw(window_data->window, last_usable_row, last_usable_column, "╯");

	for (int32_t x = 1; x < (last_usable_column); x++) { // connects the corners horizontally
		mvwprintw(window_data->window, 0, x, "─");
		mvwprintw(window_data->window, last_usable_row, x, "─");
	}

	if (title_set && (int32_t)strlen(title) < minimum_title_area) {
		wattron(window_data->window, COLOR_PAIR(1));
		mvwprintw(window_data->window, 0, 2, "%s", title);
		wattroff(window_data->window, COLOR_PAIR(1));
	}

	for (int32_t y = 1; y < (last_usable_row); y++) { // connects the corners vertically
		mvwprintw(window_data->window, y, 0, "│");
		mvwprintw(window_data->window, y, last_usable_column, "│");
	}

	return;
}

void input_handler(window_data *window_data, int32_t input) {
	switch (input) {
	case KEY_RESIZE:
		resize_handler(window_data);
		break;
	case KEY_DOWN:
		cursor_move(1);
		break;
	case KEY_UP:
		cursor_move(-1);
		break;
	default:
		break;
	}

	return;
}

void draw_table_header(WINDOW *window) {
	wattron(window, COLOR_PAIR(2));
	mvwprintw(window, 1, 2, "MAC\t\t\tIP\t\t802.1ad\t\t802.1Q\t\tLast seen");
	wattroff(window, COLOR_PAIR(2));

	return;
}

void print_network_data(WINDOW *window) {
	int32_t display_row_start = 2;

	pthread_mutex_lock(&device_data_structures_mutex);
	sync_display_limit();

	uint32_t limit = buffer.display_limit;
	if (buffer.head + limit > buffer.count) {
		limit = buffer.count - buffer.head;
	}

	for (uint32_t i = 0; i < limit; i++) {
		if (i == (uint32_t)cursor_position) {
			wattron(window, COLOR_PAIR(3));
		}

		print_network_row(window, display_row_start + i, 2, buffer.items[buffer.head + i], i == (uint32_t)cursor_position);

		if (i == (uint32_t)cursor_position) {
			wattroff(window, COLOR_PAIR(3));
		}
	}
	pthread_mutex_unlock(&device_data_structures_mutex);
	return;
}
