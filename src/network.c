#include "network.h"
#include <pthread.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>
#include <ifaddrs.h>
#include <net/if.h>

void *network_routine(void *args){
	(void)args;
	int socket_fd;
	network_init(&socket_fd);

	return NULL;
}

void network_init(int *socket_fd){
	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (*socket_fd == -1){
		perror("socket failed! try entering: sudo ./NetDet\n");
		pthread_kill(main_thread_id, SIGUSR1);
		return;
	}

	struct sockaddr_ll sll;
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = htons(ETH_P_ALL);
	sll.sll_ifindex = if_nametoindex("eth0");

	if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1){
		perror("bind");
		close(*socket_fd);
		return;
	}
}
