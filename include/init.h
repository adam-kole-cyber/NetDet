#ifndef INIT_H
#define INIT_H

#include "app_context.h"
#include "device.h"
#include "network.h"
#include "tui.h"
#include <signal.h>
#include <stdint.h>

void main_init(app_context *variables, struct network_thread_args *args);
void network_init(int32_t *socket_fd, struct network_thread_args *args, hash_map *map, int32_t *epoll_fd, int32_t *timer_fd);
void popup_init(window_data *popup_window, const window_data *main_window, pthread_t signal_thread);
void ncurses_init(void);
void signal_init(sigset_t *mask);

#endif
