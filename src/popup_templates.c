#include "popup_templates.h"
#include "error.h"
#include "scroll_view.h"
#include "shared_state.h"
#include "tui_popup.h"
#include <net/if.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static struct if_nameindex *popup_list = NULL;

static void prepare_interface_list(void *args) {
	(void)args;
	struct if_nameindex *kernel_interface = NULL;
	int32_t count = 0;
	int32_t i = 0;

	kernel_interface = if_nameindex();
	if (kernel_interface == NULL) {
		main_error(APP_ERR_IF_NAMEINDEX);
		return;
	}

	while (kernel_interface[count].if_index != 0) {
		// is used to determine how many records an array contains
		count++;
	}

	popup_list = calloc(count + 2, sizeof(struct if_nameindex));
	if (popup_list == NULL) {
		main_error(APP_ERR_CALLOC);
		return;
	}

	for (i = 0; i < count; i++) {
		popup_list[i].if_index = kernel_interface[i].if_index;
		popup_list[i].if_name = strdup(kernel_interface[i].if_name);
	}
	if_freenameindex(kernel_interface);

	popup_list[count].if_index = UINT32_MAX;
	popup_list[count].if_name = strdup("all");
	set_popup_descriptor_data_count(INTERFACES_LIST, count + 1);

	return;
}

static void interfaces_list_clean_up(void *args) {
	(void)args;
	for (int32_t i = 0; popup_list[i].if_name != NULL; i++) {
		free(popup_list[i].if_name);
		popup_list[i].if_name = NULL;
	}
	free(popup_list);
	popup_list = NULL;

	return;
}

static void render_interface_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data) {
	struct if_nameindex *items = *(struct if_nameindex **)data;

	pthread_mutex_lock(&binded_interface_mutex);
	mvwprintw(popup_window, row, col, "[%c] - %s", (binded_interface.if_index == items[index].if_index) ? '*' : ' ', items[index].if_name);
	pthread_mutex_unlock(&binded_interface_mutex);

	return;
}

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
					  .data_count = (int32_t)(sizeof(popup_inspect) / sizeof(popup_inspect[0])),
					  .data_init = prepare_inspect_data,
					  .data_cleanup = NULL,
					  .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0},
					  .render_item = render_inspect_item},
};

void set_popup_descriptor_data_count(popup_type type, int32_t count) { popup_descriptors[type].data_count = count; }
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

uint32_t get_interface_index(int32_t interface_index) { return popup_list[interface_index].if_index; }
char *get_interface_name(int32_t interface_index) { return popup_list[interface_index].if_name; }
