#ifndef UI_SCROLL_VIEW_H
#define UI_SCROLL_VIEW_H

#include <stdatomic.h>
#include <stdint.h>

typedef struct {
	uint32_t count;							 // represents the total number of items
	uint32_t head;							 // index of the first item displayed
	uint32_t visible;						 // shows how many are actually being displayed right now
	atomic_uint_least32_t viewport_capacity; // shows how many lines fit on the screen (changes when the window is resized)
	int32_t cursor;							 // cursor position within the visible window
} scroll_view;

void scroll_move(scroll_view *view, int32_t direction);
void scroll_view_configure(scroll_view *view, int32_t data_count, int32_t window_height);

#endif
