#ifndef EPOLL_UTILS_H
#define EPOLL_UTILS_H

#include <stdint.h>

void epoll_register(int32_t epoll_fd, int32_t fd_to_register);
void epoll_unregister(int32_t epoll_fd, int32_t fd_to_unregister);

#endif
