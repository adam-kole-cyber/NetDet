#ifndef TUI_POPUP_H
#define TUI_POPUP_H

#include "tui.h"
#include <pthread.h>

void popup_window_action(window_data *main_window, window_data *popup_window, pthread_t signal_thread);
void draw_popup(window_data *popup_window);

#endif
