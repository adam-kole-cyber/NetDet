#include "tui.h"
#include "device.h"
#include "network.h"
#include <bits/pthreadtypes.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>

static int cursor_position = 0;

static void print_network_row(WINDOW *window, int row, int column, const device *device_data) {
	mvwprintw(window, row, column, "%02x:%02x:%02x:%02x:%02x:%02x\t%d.%d.%d.%d\t%d\t\t%d\t\t%02d:%02d:%02d ", device_data->mac[0],
			  device_data->mac[1], device_data->mac[2], device_data->mac[3], device_data->mac[4], device_data->mac[5], device_data->ip[0],
			  device_data->ip[1], device_data->ip[2], device_data->ip[3], device_data->qinq_tag, device_data->dot1q_tag, device_data->last_seen.hour,
			  device_data->last_seen.minutes, device_data->last_seen.seconds);
	return;
}

static void resize_handler(window_data *window_data) {
	int new_height = LINES - (WINDOW_OUTER_INDENT * 2);
	int new_width = COLS - (WINDOW_OUTER_INDENT * 2);

	for (int i = 0; i < window_data->height; i++) { // cleaning leftovers from old window
		for (int j = 0; j < window_data->width; j++) {
			mvwprintw(window_data->window, i, j, " ");
		}
	}

	window_data->height = new_height;
	window_data->width = new_width;

	int computed_limit = (window_data->height - 2) - 1;

	pthread_mutex_lock(&device_data_structures_mutex);
	buffer.display_limit = (computed_limit) < 0 ? 0 : computed_limit;

	if (buffer.display_limit > buffer.capacity) {
		buffer.display_limit = buffer.capacity;
	}
	pthread_mutex_unlock(&device_data_structures_mutex);

	wnoutrefresh(window_data->window);
	doupdate();

	wresize(window_data->window, new_height, new_width);
	mvwin(window_data->window, WINDOW_OUTER_INDENT, WINDOW_OUTER_INDENT);
}

static void cursor_move(int direction) {
	int new_position = cursor_position + direction;

	pthread_mutex_lock(&device_data_structures_mutex);
	if (new_position < 0) {
		if (buffer.head > 0) {
			buffer.head--;
		}
		cursor_position = 0;
	} else if ((unsigned int)new_position >= buffer.display_limit && (buffer.head + buffer.display_limit) < buffer.capacity &&
			   buffer.items[buffer.head + buffer.display_limit] != NULL) {
		buffer.head++;
		cursor_position = buffer.display_limit - 1;
	} else {
		if (buffer.head + (unsigned int)new_position >= buffer.count) {
			return;
		}
		cursor_position = new_position;
	}
	pthread_mutex_unlock(&device_data_structures_mutex);
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
	}
}

void draw_window_frame(window_data *window_data, const char *title) {
	bool title_set = title != NULL;
	int last_usable_column = window_data->width - 1; // the window width will be reduced due to the frame
	int last_usable_row = window_data->height - 1;
	int minimum_title_area = last_usable_column - 2; // to ensure that title is not sticked together with corners

	mvwprintw(window_data->window, 0, 0, "╭"); // draw the corners of the frame
	mvwprintw(window_data->window, 0, last_usable_column, "╮");
	mvwprintw(window_data->window, last_usable_row, 0, "╰");
	mvwprintw(window_data->window, last_usable_row, last_usable_column, "╯");

	for (int x = 1; x < (last_usable_column); x++) { // connects the corners horizontally
		mvwprintw(window_data->window, 0, x, "─");
		mvwprintw(window_data->window, last_usable_row, x, "─");
	}

	if (title_set && (int)strlen(title) < minimum_title_area) {
		wattron(window_data->window, COLOR_PAIR(1));
		mvwprintw(window_data->window, 0, 2, "%s", title);
		wattroff(window_data->window, COLOR_PAIR(1));
	}

	for (int y = 1; y < (last_usable_row); y++) { // connects the corners vertically
		mvwprintw(window_data->window, y, 0, "│");
		mvwprintw(window_data->window, y, last_usable_column, "│");
	}
}

void input_handler(window_data *window_data, int input) {
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
}

void draw_table_header(WINDOW *window) {
	wattron(window, COLOR_PAIR(2));
	mvwprintw(window, 1, 2, "MAC\t\t\tIP\t\t802.1ad\t\t802.1Q\t\tLast seen");
	wattroff(window, COLOR_PAIR(2));

	return;
}

void print_network_data(WINDOW *window) {
	int display_row_start = 2;

	pthread_mutex_lock(&device_data_structures_mutex);
	unsigned int limit = buffer.display_limit;
	if (buffer.head + limit > buffer.count) {
		limit = buffer.count - buffer.head;
	}

	for (unsigned int i = 0; i < limit; i++) {
		if (i == (unsigned int)cursor_position) {
			wattron(window, COLOR_PAIR(3));
		}

		print_network_row(window, display_row_start + i, 2, buffer.items[buffer.head + i]);

		if (i == (unsigned int)cursor_position) {
			wattroff(window, COLOR_PAIR(3));
		}
	}
	pthread_mutex_unlock(&device_data_structures_mutex);
	return;
}
