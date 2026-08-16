#include "popup_templates.h"
#include "shared_state.h"

void render_interface_item(void);
void render_inspect_item(void);

static const popup_descriptor popup_descriptors[POPUP_TYPE_COUNT] = {
	[INTERFACES_LIST] = {.data = &popup_list.items, .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0}, .render_item = render_interface_item},
	[INSPECT_LIST] = {.data = &popup_inspect, .view = {.count = 0, .cursor = 0, .head = 0, .visible = 0}, .render_item = render_inspect_item},
};
