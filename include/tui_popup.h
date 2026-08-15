#ifndef TUI_POPUP_H
#define TUI_POPUP_H

#include "device.h"
#include "tui.h"
#include <pthread.h>
#include <stdint.h>

enum union_type { VLAN_TAG, ATOMIC_INT, IP, GRAPH, NONE };

typedef struct {
	const char *string;
	enum union_type getter_type;
	union {
		uint32_t (*get_vlan_tag)(void *arg);
		uint32_t (*get_atomic_int)(void *arg);
		const char *(*get_ip)(void *arg);
		const char *(*get_graph)(void *arg);
	} getter;
	void *arg;
} inspect_field_t;

void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_tpye, device *action_device,
						 pthread_t signal_thread);
void draw_popup(popup_window_data *popup_window);

#endif
