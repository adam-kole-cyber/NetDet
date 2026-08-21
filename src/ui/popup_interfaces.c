#include "ui/popup_interfaces.h"
#include "error.h"
#include "net/raw_socket.h"
#include "ui/popup_templates.h"
#include <net/if.h>
#include <stdlib.h>
#include <string.h>

struct if_nameindex *popup_list = NULL;

uint32_t get_interface_index(int32_t interface_index) { return popup_list[interface_index].if_index; }
char *get_interface_name(int32_t interface_index) { return popup_list[interface_index].if_name; }

void prepare_interface_list(void *args) {
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

void render_interface_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data) {
	struct if_nameindex *items = *(struct if_nameindex **)data;

	mvwprintw(popup_window, row, col, "[%c] - %s", (get_bound_interface() == items[index].if_index) ? '*' : ' ', items[index].if_name);

	return;
}

void interfaces_list_clean_up(void *args) {
	(void)args;
	for (int32_t i = 0; popup_list[i].if_name != NULL; i++) {
		free(popup_list[i].if_name);
		popup_list[i].if_name = NULL;
	}
	free(popup_list);
	popup_list = NULL;

	return;
}
