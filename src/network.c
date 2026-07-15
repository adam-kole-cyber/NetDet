#include "network.h"
#include "error.h"
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

atomic_bool end_listen_loop = false;

static void network_init(int *socket_fd, struct network_thread_args *args) {
	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		pthread_kill(signal_thread_id, SIGUSR1);
		pthread_exit(NULL);
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

static void process_raw_arp_frame(unsigned char *raw_frame_data, unsigned char *processed_frame, ssize_t *frame_length) {
	// Since the IEEE standards do not specify what it means when the fields for 802.1Q and 802.1ad tags are set to 0,
	// I decided that in the program this will represent a missing tag (the frame arrived without a tag),
	// since I want to maintain a uniform frame length to simplify working with them.

	if (raw_frame_data[12] == 0x08 && raw_frame_data[13] == 0x06) { // EtherType - 0x0806 -> ethernet frame
		memcpy(processed_frame, raw_frame_data, ETH_MAC_ADDRS_LEN);
		memset(&processed_frame[12], 0, ETH_QinQ_DOT1Q_TAGS_LEN);
		memcpy(&processed_frame[20], &raw_frame_data[12], *frame_length - ETH_MAC_ADDRS_LEN);
		*frame_length += 8;
	} else if (raw_frame_data[12] == 0x81 && raw_frame_data[13] == 0x00) { // TPID - 0x8100 -> 802.1Q frame
		memcpy(processed_frame, raw_frame_data, ETH_MAC_ADDRS_LEN);
		memset(&processed_frame[12], 0, ETH_QinQ_TAG_LEN);
		memcpy(&processed_frame[16], &raw_frame_data[12], *frame_length - ETH_MAC_ADDRS_LEN);
		*frame_length += 4;
	} else if (!(raw_frame_data[12] == 0x88 && raw_frame_data[13] == 0xa8)) { // TPID - 0x88a8 -> 802.1ad frame
	}
}

void *network_routine(void *args) {
	int socket_fd;

	int epoll_fd = epoll_create1(0);
	struct epoll_event ev;
	struct epoll_event events[2];

	ev.events = EPOLLIN;
	ev.data.fd = shutdown_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, shutdown_fd, &ev);

	network_init(&socket_fd, (struct network_thread_args *)args);

	ev.events = EPOLLIN;
	ev.data.fd = socket_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev);

	while (!atomic_load(&end_listen_loop)) {
		int number_of_events = epoll_wait(epoll_fd, events, 2, -1);

		for (int i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == socket_fd) {
				unsigned char raw_frame_data[2048]; // expect a standard-length frame (as defined by IEEE 802.3), but I'm still leaving some room
				unsigned char processed_frame[2048];
				ssize_t frame_length = 0;

				memset(raw_frame_data, 0, sizeof(raw_frame_data));
				memset(processed_frame, 0, sizeof(processed_frame));

				frame_length = recvfrom(socket_fd, raw_frame_data, sizeof(raw_frame_data), 0, NULL, NULL);
				if (frame_length < 0) {
					break;
				}

				process_raw_arp_frame(raw_frame_data, processed_frame, &frame_length);

			} else if (events[i].data.fd == shutdown_fd) {
				continue;
			}
		}
	}

	close(socket_fd);
	return NULL;
}
