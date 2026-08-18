#include "popup_templates.h"
#include "scroll_view.h"
#include "shared_state.h"
#include "tui_popup.h"
#include <net/if.h>
#include <stdint.h>

void render_interface_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data) {
	struct if_nameindex *items = *(struct if_nameindex **)data;

	pthread_mutex_lock(&binded_interface_mutex);
	mvwprintw(popup_window, row, col, "[%c] - %s", (binded_interface.if_index == items[index].if_index) ? '*' : ' ', items[index].if_name);
	pthread_mutex_unlock(&binded_interface_mutex);

	return;
}

void render_inspect_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data) {
	const inspect_field_t *items = (const inspect_field_t *)data;

	switch (popup_inspect[index].getter_type) {
	case IP:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_ip(items[index].arg));
		break;
	case VLAN_TAG: {
		uint32_t vlan_tag = items[index].getter.get_vlan_tag(items[index].arg);
		mvwprintw(popup_window, row, col, items[index].string, (vlan_tag & 0xfff), (vlan_tag & 0xe000) >> 13, (vlan_tag & 0x1000) >> 12);
		break;
	}
	case ATOMIC_INT:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_atomic_int(items[index].arg));
		break;
	case GRAPH:
		mvwprintw(popup_window, row, col, items[index].string, items[index].getter.get_graph(items[index].arg));
		break;
	case NONE:
		mvwprintw(popup_window, row, col, "%s", items[index].string);
		break;
	default:
		break;
	}

	return;
}

popup_descriptor popup_descriptors[POPUP_TYPE_COUNT] = {
	[INTERFACES_LIST] = {.popup_title = " Available interfaces ",
						 .data = (void *)&popup_list,
						 .data_count = 0,
						 .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0},
						 .render_item = render_interface_item},
	[INSPECT_LIST] = {.popup_title = " Inspect ",
					  .data = (void *)&popup_inspect,
					  .data_count = (int32_t)(sizeof(popup_inspect) / sizeof(popup_inspect[0])),
					  .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0},
					  .render_item = render_inspect_item},
};

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
