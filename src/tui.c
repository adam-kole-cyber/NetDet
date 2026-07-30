#include "tui.h"
#include "app_context.h"
#include "clean_up.h"
#include "device.h"
#include "init.h"
#include "shared_state.h"
#include <ncurses.h>
#include <net/if.h>
#include <panel.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int32_t cursor_main_position = 0;
int32_t cursor_popup_position = 0;
int32_t number_of_records = 0;
static int32_t interfaces_head = 0;
int32_t visible_records = 0;
struct if_nameindex *interfaces;

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
			pair = 2;
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
	wprintw(window, "\t%u.%u.%u.%u\t", ip[0], ip[1], ip[2], ip[3]);
	return;
}

static void print_qinq(WINDOW *window, const uint32_t *qinq_tag) {
	wprintw(window, "%u\t\t", (*qinq_tag & 0xfff));
	return;
}

static void print_dot1q(WINDOW *window, const uint32_t *dot1q_tag) {
	wprintw(window, "%u\t\t", (*dot1q_tag & 0xfff));
	return;
}

static void print_lastseen(WINDOW *window, const time_struct *last_seen) {
	wprintw(window, "%02u:%02u:%02u", atomic_load(&last_seen->hour), atomic_load(&last_seen->minutes), atomic_load(&last_seen->seconds));
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

static inline void sync_display_limit(sliding_window_buffer *buffer) {
	buffer->display_limit = (atomic_load(&buffer->display_row) < buffer->size) ? buffer->display_row : buffer->size;
}

static void cursor_move(sliding_window_buffer *buffer, int32_t direction) {
	int32_t new_position = cursor_main_position + direction;

	sync_display_limit(buffer);

	if (new_position < 0) {
		if (buffer->head > 0) {
			buffer->head--;
		}
		cursor_main_position = 0;
	} else if ((uint32_t)new_position >= buffer->display_limit && (buffer->head + buffer->display_limit) < buffer->count) {
		buffer->head++;
		cursor_main_position = buffer->display_limit - 1;
	} else {
		if (buffer->head + (uint32_t)new_position >= buffer->count) {
			return;
		}
		cursor_main_position = new_position;
	}

	return;
}

static void cursor_popup_move(window_data *popup_window, int32_t direction) {
	(void)popup_window;

	if (number_of_records == 0)
		return;

	int32_t records_on_screen = number_of_records - interfaces_head;

	if (records_on_screen > visible_records)
		records_on_screen = visible_records;

	int32_t new_position = cursor_popup_position + direction;

	if (new_position < 0) {
		if (interfaces_head > 0) {
			interfaces_head--;
		}

		cursor_popup_position = 0;
		return;
	}

	if (new_position >= records_on_screen) {
		if (interfaces_head + records_on_screen < number_of_records) {
			interfaces_head++;
		}

		cursor_popup_position = records_on_screen - 1;
		return;
	}

	cursor_popup_position = new_position;
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

void input_handler(int32_t input, app_context *variables) {
	switch (input) {
	case KEY_DOWN:
		if (variables->main_window.is_active) {
			cursor_move(&variables->buffer, 1);
		} else {
			cursor_popup_move(&variables->popup_window, 1);
		}
		break;
	case KEY_UP:
		if (variables->main_window.is_active) {
			cursor_move(&variables->buffer, -1);
		} else {
			cursor_popup_move(&variables->popup_window, -1);
		}
		break;
	case 'b':
		popup_window_action(&variables->main_window, &variables->popup_window, variables->signal_thread);
		break;
	case '\n':
		if (variables->popup_window.is_active) {
			int32_t selected = interfaces_head + cursor_popup_position;

			if (selected >= number_of_records)
				break;
			if (interfaces[selected].if_index == 0)
				break;

			binded_interface.if_index = interfaces[selected].if_index;
			binded_interface.if_name = interfaces[selected].if_name;

			popup_window_action(&variables->main_window, &variables->popup_window, variables->signal_thread);
		}
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

void print_network_data(WINDOW *window, sliding_window_buffer *buffer) {
	int32_t display_row_start = 2;

	sync_display_limit(buffer);

	uint32_t limit = buffer->display_limit;
	if (buffer->head + limit > buffer->count) {
		limit = buffer->count - buffer->head;
	}

	for (uint32_t i = 0; i < limit; i++) {
		if (i == (uint32_t)cursor_main_position) {
			wattron(window, COLOR_PAIR(3));
		}

		print_network_row(window, display_row_start + i, 2, buffer->items[buffer->head + i], i == (uint32_t)cursor_main_position);

		if (i == (uint32_t)cursor_main_position) {
			wattroff(window, COLOR_PAIR(3));
		}
	}
	return;
}

void draw(app_context *variables) {
	if (variables->main_window.height < MIN_HEIGHT || variables->main_window.width < MIN_WIDTH) {
		werase(stdscr);
		wattron(stdscr, COLOR_PAIR(4));
		mvwprintw(stdscr, 0, 0, "Window is too small!");
		wattroff(stdscr, COLOR_PAIR(4));
		wnoutrefresh(stdscr);
		doupdate();
	} else {
		werase(stdscr);
		wnoutrefresh(stdscr);
		werase(variables->main_window.window);
		draw_window_frame(&variables->main_window, " NetDet ");
		draw_table_header(variables->main_window.window);
		print_network_data(variables->main_window.window, &variables->buffer);
		mvwprintw(variables->main_window.window, 0, 10, "%d %d %d %d", variables->popup_window.start_x, variables->popup_window.start_y,
				  variables->popup_window.width, variables->popup_window.height);

		if (variables->popup_window.is_active) {
			werase(variables->popup_window.window);
			draw_window_frame(&variables->popup_window, " Available interfaces ");
			draw_popup(&variables->popup_window);
		}

		update_panels();
		doupdate();
	}
	return;
}

void popup_window_action(window_data *main_window, window_data *popup_window, pthread_t signal_thread) {
	static bool is_visible = false;

	is_visible = !is_visible;

	if (is_visible) {
		main_window->is_active = false;

		popup_init(popup_window, main_window, signal_thread);

		number_of_records = 0;
		while (interfaces[number_of_records].if_index != 0) {
			number_of_records++;
		}

		visible_records = popup_window->height - 2;

		if (visible_records > number_of_records) {
			visible_records = number_of_records;
		}

		interfaces_head = 0;
		cursor_popup_position = 0;

		draw_window_frame(popup_window, " Available interfaces ");
		draw_popup(popup_window);
	} else {
		main_window->is_active = true;
		popup_clean_up(popup_window);
	}

	return;
}

void draw_popup(window_data *popup_window) {
	for (int32_t i = 0; i < visible_records; i++) {
		int32_t index = interfaces_head + i;

		if (index >= number_of_records) {
			break;
		}

		if (cursor_popup_position == i) {
			wattron(popup_window->window, COLOR_PAIR(3));
		}

		mvwprintw(popup_window->window, 1 + i, 2, "[%c] - %s", (binded_interface.if_index == interfaces[index].if_index) ? '*' : ' ',
				  interfaces[index].if_name);

		if (cursor_popup_position == i) {
			wattroff(popup_window->window, COLOR_PAIR(3));
		}
	}

	return;
}
