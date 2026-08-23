#include "tui.h"
#include "app_context.h"
#include "core/device.h"
#include "net/raw_socket.h"
#include "scroll_view.h"
#include "tui_app.h"
#include "tui_popup.h"
#include "ui/popup_interfaces.h"
#include "ui/popup_templates.h"
#include <ncurses.h>
#include <net/if.h>
#include <panel.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

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
			scroll_move(get_popup_descriptor_scroll_view(variables->popup_window.popup_type), 1);
		}
		break;
	case KEY_UP:
		if (variables->main_window.is_active) {
			scroll_move(&variables->buffer.view, -1);
		} else {
			scroll_move(get_popup_descriptor_scroll_view(variables->popup_window.popup_type), -1);
		}
		break;
	case 'b':
		popup_window_action(&variables->main_window, &variables->popup_window, INTERFACES_LIST, NULL);
		break;
	case 'i': {
		device_table_view *buffer_reference = &variables->buffer;
		device *action_device = buffer_reference->data->items[buffer_reference->view.head + buffer_reference->view.cursor];
		if (action_device == NULL) {
			break;
		}
		// the way in wich is action device passed to function might be reworked later
		popup_window_action(&variables->main_window, &variables->popup_window, INSPECT_LIST, action_device);
		break;
	}
	case '\n':
		if (variables->popup_window.is_active && variables->popup_window.popup_type == INTERFACES_LIST) {
			scroll_view *view = get_popup_descriptor_scroll_view(variables->popup_window.popup_type);
			int32_t selected = view->head + view->cursor;

			if ((uint32_t)selected >= view->count)
				break;

			if (get_interface_index(selected) == 0)
				break;

			set_bound_interface(get_interface_index(selected), get_interface_name(selected));
			bind_update_notify();

			popup_window_action(&variables->main_window, &variables->popup_window, INTERFACES_LIST, NULL);
		}

		break;
	default:
		break;
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
			draw_window_frame((window_data *)&variables->popup_window, get_popup_descriptor_title(variables->popup_window.popup_type));
			draw_popup(&variables->popup_window);
		}

		update_panels();
		doupdate();
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

	set_column_width(variables->main_window.width);

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

		popup_descriptor *descriptor = get_popup_descriptor(variables->popup_window.popup_type);
		scroll_view_configure(&descriptor->view, descriptor->data_count, variables->popup_window.height);

		wresize(variables->popup_window.window, variables->popup_window.height, variables->popup_window.width);
		move_panel(variables->popup_window.panel, variables->popup_window.start_y, variables->popup_window.start_x);
	}

	return;
}
