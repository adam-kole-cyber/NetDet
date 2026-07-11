#include "network.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *network_routine(void *args) {
	int socket_fd;
	network_init(&socket_fd, (struct network_thread_args *)args);

	return NULL;
}

void network_init(int *socket_fd, struct network_thread_args *args) {
	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (*socket_fd == -1) {
		perror("socket failed! try entering: sudo ./NetDet\n");
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
			perror("the specified interface does not exist");
			pthread_kill(main_thread_id, SIGUSR1);
			return;
		}

		if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
			perror("bind");
			close(*socket_fd);
			return;
		}
	}
}
