#include "scroll_view.h"

void scroll_move(scroll_view *view, int32_t direction) {
	if (view->count == 0)
		return;

	int32_t records_on_screen = view->count - view->head;
	int32_t new_position = view->cursor + direction;

	if ((uint32_t)records_on_screen > view->visible)
		records_on_screen = view->visible;

	if (new_position < 0) {
		if (view->head > 0) {
			view->head--;
		}

		view->cursor = 0;
		return;
	}

	if (new_position >= records_on_screen) {
		if (view->head + records_on_screen < view->count) {
			view->head++;
		}

		view->cursor = records_on_screen - 1;
		return;
	}

	view->cursor = new_position;
	return;
}
