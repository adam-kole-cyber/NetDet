#ifndef NET_NETWORKTHREAD_H
#define NET_NETWORKTHREAD_H

#include "core/device.h"
#include <stdint.h>

struct network_thread_args {
	int argc;
	char **argv;
};

void network_init(int32_t *socket_fd, int32_t *epoll_fd, int32_t *timer_fd, hash_map *map, struct network_thread_args *args);
void *network_routine(void *args);
void network_request_shutdown(void);
void network_clean_up(int32_t *socket_fd, int32_t *epoll_fd, int32_t *timer_fd, hash_map *map);

#endif
