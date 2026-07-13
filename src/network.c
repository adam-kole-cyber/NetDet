#include "network.h"
#include "error.h"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/types/sigset_t.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <ifaddrs.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

volatile sig_atomic_t end_listen_loop = false;

static void network_init(int *socket_fd, struct network_thread_args *args) {
	struct timeval tv = {.tv_sec = 0, .tv_usec = 200000};

	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		pthread_kill(main_thread_id, SIGUSR1);
		pthread_exit(NULL);
		return;
	}

	if (setsockopt(*socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
		network_error(APP_ERR_SETSOCKOPT, socket_fd);
		return;
	}

	if (args->argc > 1) {
		struct sockaddr_ll sll;
		memset(&sll, 0, sizeof(sll));
		sll.sll_family = AF_PACKET;
		sll.sll_protocol = htons(ETH_P_ALL);
		sll.sll_ifindex = if_nametoindex(args->argv[1]);

		if (sll.sll_ifindex == 0) {
			network_error(APP_ERR_IF_NAMETOINDEX, socket_fd);
			return;
		}

		if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
			network_error(APP_ERR_BIND, socket_fd);
			return;
		}
	}
}

void *network_routine(void *args) {
	int socket_fd;
	char raw_frame_data[2048]; // expect a standard-length frame (as defined by IEEE 802.3), but I'm still leaving some room
	ssize_t received_length = 0;
	network_init(&socket_fd, (struct network_thread_args *)args);

	while (!end_listen_loop) {
		received_length = recvfrom(socket_fd, raw_frame_data, sizeof(raw_frame_data), 0, NULL, NULL);
		if (received_length < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			break;
		}
	}

	close(socket_fd);
	return NULL;
}
