#include "network.h"
#include "error.h"
#include <arpa/inet.h>
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
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void network_init(int *socket_fd, struct network_thread_args *args) {
	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		pthread_kill(main_thread_id, SIGUSR1);
		return;
	}

	if (args->argc > 1) {
		struct sockaddr_ll sll;
		memset(&sll, 0, sizeof(sll));
		sll.sll_family = AF_PACKET;
		sll.sll_protocol = htons(ETH_P_ALL);
		sll.sll_ifindex = if_nametoindex(args->argv[1]);

		if (sll.sll_ifindex == 0) {
			set_error(APP_ERR_IF_NAMETOINDEX, errno);
			close(*socket_fd);
			pthread_kill(main_thread_id, SIGUSR1);
			return;
		}

		if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
			set_error(APP_ERR_BIND, errno);
			close(*socket_fd);
			pthread_kill(main_thread_id, SIGUSR1);
			return;
		}
	}
}

void *network_routine(void *args) {
	int socket_fd;
	network_init(&socket_fd, (struct network_thread_args *)args);

	return NULL;
}
