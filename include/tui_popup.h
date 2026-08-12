#ifndef TUI_POPUP_H
#define TUI_POPUP_H

#include "tui.h"
#include <pthread.h>

typedef struct {
	const char *string;
	union {
		int (*get_int)(void);
		char (*get_char)(void);
		const char *(*get_ip)(void *arg);
		const char *(*get_str)(void);
	} getter;
	void *arg;
} inspect_field_t;

void popup_window_action(window_data *main_window, window_data *popup_window, pthread_t signal_thread);
void draw_popup(window_data *popup_window);

#endif
