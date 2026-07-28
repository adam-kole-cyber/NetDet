#ifndef CLEAN_UP_H
#define CLEAN_UP_H

#include "app_context.h"
#include "tui.h"

void main_clean_up(app_context *variables);
void network_clean_up(hash_map *map, int32_t *socket_fd, int32_t *epoll_fd);
void popup_clean_up(window_data *popup_window);

#endif
