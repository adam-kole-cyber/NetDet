#include "ui/tui_popup.h"
#include "core/device.h"
#include "ui/popup_templates.h"
#include "ui/scroll_view.h"
#include "ui/tui.h"
#include <ncurses.h>
#include <panel.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

void popup_init(popup_window_data *popup_window, const window_data *main_window, popup_type window_type) {
	popup_window->is_active = true;
	popup_window->popup_type = window_type;

	if ((main_window->height - 10) >= 3) {
		popup_window->start_x = main_window->start_x + 5;
		popup_window->start_y = main_window->start_y + 5;
		popup_window->width = main_window->width - 10;
		popup_window->height = main_window->height - 10;
	} else {
		popup_window->start_x = main_window->start_x;
		popup_window->start_y = main_window->start_y;
		popup_window->width = main_window->width;
		popup_window->height = main_window->height;
	}

	popup_window->window =
		newwin(popup_window->height, popup_window->width, popup_window->start_y, popup_window->start_x);
	popup_window->panel = new_panel(popup_window->window);

	return;
}

void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_type,
						 device *action_device) {
	static bool is_visible = true;

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
		if (popup_window->is_active && popup_window->popup_type != window_type) {
			return;
		}

		main_window->is_active = true;
		popup_clean_up(popup_window);

		popup_descriptor *descriptor = get_popup_descriptor(popup_window->popup_type);

		if (descriptor->data_cleanup != NULL) {
			descriptor->data_cleanup(descriptor->args);
		}
	}

	is_visible = !is_visible;
	return;
}

void popup_clean_up(popup_window_data *popup_window) {
	popup_window->is_active = false;
	popup_window->start_x = 0;
	popup_window->start_y = 0;
	popup_window->height = 0;
	popup_window->width = 0;

	del_panel(popup_window->panel);
	delwin(popup_window->window);
	popup_window->panel = NULL;
	popup_window->window = NULL;

	return;
}
