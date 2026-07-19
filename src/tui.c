#include "tui.h"
#include "device.h"
#include "network.h"
#include <bits/pthreadtypes.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>

static int cursor_position = 0;

static void print_mac(WINDOW *window, int row, int column, const unsigned char *mac) {
	mvwprintw(window, row, column, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return;
}

static void print_ip(WINDOW *window, int row, int column, const unsigned char *ip) {
	mvwprintw(window, row, column, "%02d.%02d.%02d.%02d", ip[0], ip[1], ip[2], ip[3]);
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

	buffer.display_limit = (window_data->height - 2) < 0 ? 0 : (window_data->height - 2) - 1;

	wnoutrefresh(window_data->window);
	doupdate();

	wresize(window_data->window, new_height, new_width);
	mvwin(window_data->window, WINDOW_OUTER_INDENT, WINDOW_OUTER_INDENT);
}

static void cursor_move(int direction) {
	int new_position = cursor_position + direction;

	if (new_position < 0) {
		if (buffer.head > 0) {
			buffer.head--;
		}
		cursor_position = 0;
	} else if ((unsigned int)new_position > buffer.display_limit && buffer.items[buffer.head + buffer.display_limit] != NULL) {
		buffer.head++;
		cursor_position = buffer.display_limit;
	} else {
		if (buffer.head + (unsigned int)new_position >= buffer.count) {
			return;
		}
		cursor_position = new_position;
	}
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
	unsigned int limit = buffer.display_limit <= buffer.count ? buffer.display_limit : buffer.count;

	pthread_mutex_lock(&device_data_structures_mutex);
	for (unsigned int i = 0; i < limit; i++) {
		if (i == (unsigned int)cursor_position) {
			wattron(window, COLOR_PAIR(3));
		}
		print_mac(window, display_row_start + i, 2, buffer.items[buffer.head + i]->mac);
		print_ip(window, display_row_start + i, 24, buffer.items[buffer.head + i]->ip);
		mvwprintw(window, display_row_start + i, 40, "%d\t\t%d\t\t%02d:%02d:%02d", buffer.items[buffer.head + i]->qinq_tag,
				  buffer.items[buffer.head + i]->dot1q_tag, buffer.items[buffer.head + i]->last_seen.hour,
				  buffer.items[buffer.head + i]->last_seen.minutes, buffer.items[buffer.head + i]->last_seen.seconds);
		mvwprintw(window, display_row_start + i, 90, "%d", limit);
		if (i == (unsigned int)cursor_position) {
			wattroff(window, COLOR_PAIR(3));
		}
	}
	pthread_mutex_unlock(&device_data_structures_mutex);
	return;
}
