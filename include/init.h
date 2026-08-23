#ifndef INIT_H
#define INIT_H

#include "app_context.h"
#include "net/network_thread.h"
#include "ui/tui.h"
#include <signal.h>
#include <stdint.h>

void main_init(app_context *variables, struct network_thread_args *args);
void popup_init(popup_window_data *popup_window, const window_data *main_window, popup_type window_type);
void ncurses_init(void);
void signal_init(sigset_t *mask);

#endif
