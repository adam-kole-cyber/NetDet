#include "ui/popup_templates.h"
#include "scroll_view.h"
#include "ui/popup_inspect.h"
#include "ui/popup_interfaces.h"
#include <net/if.h>
#include <stdint.h>
#include <string.h>

popup_descriptor popup_descriptors[POPUP_TYPE_COUNT] = {
	[INTERFACES_LIST] = {.popup_title = " Available interfaces ",
						 .data = (void *)&popup_list,
						 .data_count = 0,
						 .data_init = prepare_interface_list,
						 .data_cleanup = interfaces_list_clean_up,
						 .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0},
						 .render_item = render_interface_item},
	[INSPECT_LIST] = {.popup_title = " Inspect ",
					  .data = (void *)&popup_inspect,
					  .data_count = INSPECT_FIELD_COUNT,
					  .data_init = prepare_inspect_data,
					  .data_cleanup = NULL,
					  .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0},
					  .render_item = render_inspect_item},
};

void set_popup_descriptor_data_count(popup_type type, int32_t count) {
	popup_descriptors[type].data_count = count;
	return;
}

scroll_view *get_popup_descriptor_scroll_view(popup_type type) { return &popup_descriptors[type].view; }
const char *get_popup_descriptor_title(popup_type type) { return popup_descriptors[type].popup_title; }
popup_descriptor *get_popup_descriptor(popup_type type) { return &popup_descriptors[type]; }

void draw_popup(popup_window_data *popup_window) {
	popup_descriptor *descriptor = &popup_descriptors[popup_window->popup_type];
	scroll_view *data_scroll_view = &descriptor->view;

	for (uint32_t i = 0; i < data_scroll_view->visible; i++) {
		int32_t index = data_scroll_view->head + i;

		if ((uint32_t)index >= data_scroll_view->count) {
			break;
		}

		if ((uint32_t)data_scroll_view->cursor == i) {
			wattron(popup_window->window, COLOR_PAIR(3));
		}

		descriptor->render_item(popup_window->window, i + 1, 2, index, descriptor->data);

		if ((uint32_t)data_scroll_view->cursor == i) {
			wattroff(popup_window->window, COLOR_PAIR(3));
		}
	}

	return;
}
