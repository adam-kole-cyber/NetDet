#ifndef POPUP_TEMPLATES
#define POPUP_TEMPLATES

#include "scroll_view.h"
#include <ncurses.h>
#include <panel.h>
#include <stdint.h>

typedef enum { INTERFACES_LIST, INSPECT_LIST, POPUP_TYPE_COUNT } popup_type;

typedef struct {
	bool is_active;
	int32_t width;
	int32_t height;
	int32_t start_x;
	int32_t start_y;
	WINDOW *window;
	PANEL *panel;
	popup_type popup_type;
} popup_window_data;

typedef void (*popup_item_renderer)(void);

typedef struct {
	scroll_view view;
	popup_item_renderer render_item;
	void *data;
} popup_descriptor;

#endif
