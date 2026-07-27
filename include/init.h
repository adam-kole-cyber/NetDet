#ifndef INIT_H
#define INIT_H

#include "app_context.h"
#include "device.h"
#include "network.h"
#include <stdint.h>

void main_init(app_context *variables, struct network_thread_args *args);
void network_init(int32_t *socket_fd, struct network_thread_args *args, hash_map *map, int32_t *epoll_fd);

#endif
