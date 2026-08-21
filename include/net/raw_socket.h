#ifndef NET_RAWSOCKET_H
#define NET_RAWSOCKET_H

#include "network_thread.h"
#include <stdint.h>

int32_t raw_socket_create(void);
void socket_init(int32_t socket_fd, int32_t epoll_fd, struct network_thread_args *args);
void socket_cleanup(void);
uint32_t get_bound_interface(void);
void set_bound_interface(int32_t if_index, char *if_name);
int32_t get_bind_update_fd(void);
void bind_update_notify(void);
void change_bind(int32_t *socket_fd, int32_t *epoll);

#endif
