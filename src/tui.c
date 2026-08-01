#include "tui.h"
#include "app_context.h"
#include "clean_up.h"
#include "device.h"
#include "init.h"
#include "scroll_view.h"
#include "shared_state.h"
#include <ncurses.h>
#include <net/if.h>
#include <panel.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

interfaces_list popup_list = {0};

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
	buffer->view.visible =
		(atomic_load(&buffer->view.viewport_capacity) < buffer->size) ? atomic_load(&buffer->view.viewport_capacity) : buffer->size;
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
			scroll_move(&variables->buffer.view, 1);
		} else {
			scroll_move(&popup_list.view, 1);
		}
		break;
	case KEY_UP:
		if (variables->main_window.is_active) {
			scroll_move(&variables->buffer.view, -1);
		} else {
			scroll_move(&popup_list.view, -1);
		}
		break;
	case 'b':
		popup_window_action(&variables->main_window, &variables->popup_window, variables->signal_thread);
		break;
	case '\n':
		if (variables->popup_window.is_active) {
			int32_t selected = popup_list.view.head + popup_list.view.cursor;

			if ((uint32_t)selected >= popup_list.view.count)
				break;
			if (popup_list.items[selected].if_index == 0)
				break;

			pthread_mutex_lock(&binded_interface_mutex);
			binded_interface.if_index = popup_list.items[selected].if_index;
			free(binded_interface.if_name);
			binded_interface.if_name = strdup(popup_list.items[selected].if_name);
			pthread_mutex_unlock(&binded_interface_mutex);

			uint64_t event_updated_bind = 1;
			write(bind_update_fd, &event_updated_bind, sizeof(event_updated_bind));

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

	uint32_t limit = buffer->view.visible;
	if (buffer->view.head + limit > buffer->view.count) {
		limit = buffer->view.count - buffer->view.head;
	}

	for (uint32_t i = 0; i < limit; i++) {
		if (i == (uint32_t)buffer->view.cursor) {
			wattron(window, COLOR_PAIR(3));
		}

		print_network_row(window, display_row_start + i, 2, buffer->items[buffer->view.head + i], i == (uint32_t)buffer->view.cursor);

		if (i == (uint32_t)buffer->view.cursor) {
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

		popup_list.view.count = 0;
		while (popup_list.items[popup_list.view.count].if_index != 0) {
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

void resize_handler(app_context *variables) {
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	resizeterm(ws.ws_row, ws.ws_col);

	variables->main_window.height = LINES - (WINDOW_OUTER_INDENT * 2);
	variables->main_window.width = COLS - (WINDOW_OUTER_INDENT * 2);
	wresize(variables->main_window.window, variables->main_window.height, variables->main_window.width);
	move_panel(variables->main_window.panel, variables->main_window.start_y, variables->main_window.start_x);

	uint32_t i = (variables->main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) < 0
					 ? 0
					 : (variables->main_window.height - WINDOW_UNUSABLE_NUMBERS_OF_LINES) - 1;

	atomic_store(&variables->buffer.view.viewport_capacity, i);

	if (variables->popup_window.is_active) {
		if ((variables->main_window.height - 10) >= 3) {
			variables->popup_window.start_x = variables->main_window.start_x + 5;
			variables->popup_window.start_y = variables->main_window.start_y + 5;
			variables->popup_window.width = variables->main_window.width - 10;
			variables->popup_window.height = variables->main_window.height - 10;
		} else {
			variables->popup_window.start_x = variables->main_window.start_x;
			variables->popup_window.start_y = variables->main_window.start_y;
			variables->popup_window.width = variables->main_window.width;
			variables->popup_window.height = variables->main_window.height;
		}

		popup_list.view.count = 0;
		while (popup_list.items[popup_list.view.count].if_index != 0) {
			popup_list.view.count++;
		}

		popup_list.view.visible = variables->popup_window.height - 2;

		if (popup_list.view.visible > popup_list.view.count) {
			popup_list.view.visible = popup_list.view.count;
		}

		wresize(variables->popup_window.window, variables->popup_window.height, variables->popup_window.width);
		move_panel(variables->popup_window.panel, variables->popup_window.start_y, variables->popup_window.start_x);
	}

	return;
}
