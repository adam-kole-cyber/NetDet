#include "net/raw_socket.h"
#include "common/error.h"
#include "epoll_utils.h"
#include "lifecycle.h"
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

static int32_t bind_update_fd;
static struct if_nameindex binded_interface = {0};
static pthread_mutex_t binded_interface_mutex;

int32_t raw_socket_create(void) {
	int32_t enable = 1;
	int32_t socket_fd;

	socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		lifecycle_notify_fatal_error();
		pthread_exit(NULL);
		return -1;
	}

	if (setsockopt(socket_fd, SOL_PACKET, PACKET_AUXDATA, &enable, sizeof(enable)) == -1) {
		network_error(APP_ERR_SETSOCKOPT, socket_fd);
		return -1;
	}

	return socket_fd;
}

void socket_init(int32_t socket_fd, int32_t epoll_fd, const char *initial_interface) {
	bind_update_fd = eventfd(0, 0);

	pthread_mutex_init(&binded_interface_mutex, NULL);

	pthread_mutex_lock(&binded_interface_mutex);
	binded_interface.if_index = UINT32_MAX;
	binded_interface.if_name = strdup("all");
	pthread_mutex_unlock(&binded_interface_mutex);

	if (initial_interface != NULL) {
		struct sockaddr_ll sll;
		memset(&sll, 0, sizeof(sll));
		sll.sll_family = AF_PACKET;
		sll.sll_protocol = htons(ETH_P_ALL);
		sll.sll_ifindex = if_nametoindex(initial_interface);

		if (sll.sll_ifindex == 0) {
			network_error(APP_ERR_IF_NAMETOINDEX, socket_fd);
			return;
		}

		if (bind(socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
			network_error(APP_ERR_BIND, socket_fd);
			return;
		}

		pthread_mutex_lock(&binded_interface_mutex);
		binded_interface.if_index = sll.sll_ifindex;
		free(binded_interface.if_name);
		binded_interface.if_name = strdup(initial_interface);
		pthread_mutex_unlock(&binded_interface_mutex);
	}

	epoll_register(epoll_fd, bind_update_fd);
	epoll_register(epoll_fd, socket_fd);

	return;
}

void socket_cleanup(void) {
	pthread_mutex_lock(&binded_interface_mutex);
	free(binded_interface.if_name);
	pthread_mutex_unlock(&binded_interface_mutex);

	pthread_mutex_destroy(&binded_interface_mutex);

	close(bind_update_fd);
	return;
}

uint32_t get_bound_interface(void) {
	pthread_mutex_lock(&binded_interface_mutex);
	uint32_t if_index = binded_interface.if_index;
	pthread_mutex_unlock(&binded_interface_mutex);

	return if_index;
}

void set_bound_interface(int32_t if_index, char *if_name) {
	pthread_mutex_lock(&binded_interface_mutex);

	binded_interface.if_index = if_index;
	free(binded_interface.if_name);

	binded_interface.if_name = strdup(if_name);

	pthread_mutex_unlock(&binded_interface_mutex);
	return;
}

int32_t get_bind_update_fd(void) { return bind_update_fd; }

void bind_update_notify(void) {
	uint64_t event_updated_bind = 1;
	write(bind_update_fd, &event_updated_bind, sizeof(event_updated_bind));

	return;
}

void change_bind(int32_t *socket_fd, int32_t *epoll) {
	int32_t enable = 1;
	struct sockaddr_ll sll;
	memset(&sll, 0, sizeof(sll));

	epoll_unregister(*epoll, *socket_fd);
	close(*socket_fd);

	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		lifecycle_notify_fatal_error();
		pthread_exit(NULL);
		return;
	}

	if (setsockopt(*socket_fd, SOL_PACKET, PACKET_AUXDATA, &enable, sizeof(enable)) == -1) {
		network_error(APP_ERR_SETSOCKOPT, *socket_fd);
		return;
	}

	pthread_mutex_lock(&binded_interface_mutex);
	if (binded_interface.if_index == UINT32_MAX && strcmp(binded_interface.if_name, "all") == 0) {
		epoll_register(*epoll, *socket_fd);
		pthread_mutex_unlock(&binded_interface_mutex);
		return;
	}
	sll.sll_ifindex = binded_interface.if_index;
	pthread_mutex_unlock(&binded_interface_mutex);

	sll.sll_family = AF_PACKET;
	sll.sll_protocol = htons(ETH_P_ALL);

	if (sll.sll_ifindex == 0) {
		network_error(APP_ERR_IF_NAMETOINDEX, *socket_fd);
		return;
	}

	if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
		network_error(APP_ERR_BIND, *socket_fd);
		return;
	}

	epoll_register(*epoll, *socket_fd);

	return;
}
