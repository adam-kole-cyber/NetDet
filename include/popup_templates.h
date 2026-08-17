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

typedef void (*popup_item_renderer)(WINDOW *popup_window, int32_t row, int32_t col, uint32_t index, void *data);

typedef struct {
	const char *popup_title;
	void *data;
	int32_t data_count;
	scroll_view view;
	popup_item_renderer render_item;
} popup_descriptor;

void draw_popup(popup_window_data *popup_window);

#endif
