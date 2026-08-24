#ifndef UI_POPUP_INSPECT_H
#define UI_POPUP_INSPECT_H

#include <ncurses.h>
#include <stdint.h>

#define INSPECT_FIELD_COUNT 6

enum union_type { VLAN_TAG, ATOMIC_INT, IP, GRAPH, RATE };

typedef struct {
	const char *string;
	enum union_type getter_type;
	union {
		uint32_t (*get_vlan_tag)(void *arg);
		uint64_t (*get_atomic_int64)(void *arg);
		uint32_t (*get_rate)(void *arg);
		const char *(*get_ip)(void *arg);
		const char *(*get_graph)(void *arg);
	} getter;
	void *arg;
} inspect_field_t;

extern const inspect_field_t popup_inspect[INSPECT_FIELD_COUNT];

void prepare_inspect_data(void *args);
void render_inspect_item(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data);

#endif
