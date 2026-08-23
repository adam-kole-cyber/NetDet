#include "tui_popup.h"
#include "clean_up.h"
#include "core/device.h"
#include "init.h"
#include "scroll_view.h"
#include "tui.h"
#include "ui/popup_templates.h"
#include <ncurses.h>
#include <net/if.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

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
