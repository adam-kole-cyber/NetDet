#include "epoll_utils.h"
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>

void epoll_register(int32_t epoll_fd, int32_t fd_to_register) {
	struct epoll_event register_event;

	register_event.events = EPOLLIN;
	register_event.data.fd = fd_to_register;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_to_register, &register_event);

	return;
}

void epoll_unregister(int32_t epoll_fd, int32_t fd_to_unregister) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_to_unregister, NULL);
	return;
}
