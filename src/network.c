#include "network.h"
#include "clean_up.h"
#include "device.h"
#include "epoll_utils.h"
#include "error.h"
#include "init.h"
#include "shared_state.h"
#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

atomic_bool end_listen_loop = false;
int32_t shutdown_network_fd;
int32_t bind_update_fd;
struct if_nameindex binded_interface = {0};
pthread_mutex_t binded_interface_mutex;

static void get_vlan_info(struct msghdr *control_msg, unsigned char *processed_frame, int32_t *socket_fd, pthread_t signal_thread) {

	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(control_msg); cmsg != NULL; cmsg = CMSG_NXTHDR(control_msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_PACKET && cmsg->cmsg_type == PACKET_AUXDATA) {
			struct tpacket_auxdata *aux = (struct tpacket_auxdata *)CMSG_DATA(cmsg);

#ifdef TP_STATUS_VLAN_TPID_VALID
			if (aux->tp_status & TP_STATUS_VLAN_VALID) {		  // VLAN tag is present
				if (aux->tp_status & TP_STATUS_VLAN_TPID_VALID) { // TPID is available
					uint16_t offset = 0;
					uint16_t tpid = aux->tp_vlan_tpid;

					if (tpid == 0x88a8) {
						offset = 12;
					} else if (tpid == 0x8100) {
						offset = 16;
					} else {
						return;
					}

					uint16_t tpid_network_orded = htons(tpid);
					uint16_t tci_network_order = htons(aux->tp_vlan_tci);

					memcpy(&processed_frame[offset], &tpid_network_orded, sizeof(tpid_network_orded));
					memcpy(&processed_frame[offset + 2], &tci_network_order, sizeof(tci_network_order));
				} else { // TPID is unavailable (most likely due to RX VLAN OFFLOAD)
					network_error(APP_ERR_ANCILLARY_DATA, socket_fd, signal_thread);
					return;
				}

			} else { // frame was not tagged continue
				continue;
			}
#else
			// rx offload off or rebuild against newer Linux headers
			network_error(APP_ERR_ANCILLARY_DATA, socket_fd, signal_thread);
#endif
		}
	}

	return;
}

static void change_bind(int32_t *socket_fd, int32_t *epoll, pthread_t signal_thread) {
	int32_t enable = 1;
	struct sockaddr_ll sll;
	memset(&sll, 0, sizeof(sll));

	epoll_unregister(*epoll, *socket_fd);
	close(*socket_fd);

	*socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (*socket_fd == -1) {
		set_error(APP_ERR_SOCKET, errno);
		pthread_kill(signal_thread, SIGUSR1);
		pthread_exit(NULL);
		return;
	}

	if (setsockopt(*socket_fd, SOL_PACKET, PACKET_AUXDATA, &enable, sizeof(enable)) == -1) {
		network_error(APP_ERR_SETSOCKOPT, socket_fd, signal_thread);
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
		network_error(APP_ERR_IF_NAMETOINDEX, socket_fd, signal_thread);
		return;
	}

	if (bind(*socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
		network_error(APP_ERR_BIND, socket_fd, signal_thread);
		return;
	}

	epoll_register(*epoll, *socket_fd);

	return;
}

static void process_raw_arp_frame(unsigned char *raw_frame_data, unsigned char *processed_frame, ssize_t *frame_length) {
	// Since the IEEE standards do not specify what it means when the fields for 802.1Q and 802.1ad tags are set to 0,
	// I decided that in the program this will represent a missing tag (the frame arrived without a tag),
	// since I want to maintain a uniform frame length to simplify working with them.

	if (raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED] == 0x08 && raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED + 1] == 0x06) {
		memcpy(processed_frame, raw_frame_data, ETH_MAC_ADDRS_LEN);
		memset(&processed_frame[ETH_TYPE_OFFSET_UNTAGGED], 0, ETH_QinQ_DOT1Q_TAGS_LEN);
		memcpy(&processed_frame[ETH_TYPE_OFFSET_DOUBLE_TAG], &raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED], *frame_length - ETH_MAC_ADDRS_LEN);

		*frame_length += ETH_QinQ_DOT1Q_TAGS_LEN;
	} else if (raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED] == 0x81 && raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED + 1] == 0x00) {
		if (!(raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG] == 0x08 && raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG + 1] == 0x06)) {
			*frame_length = 0;
			return;
		}

		memcpy(processed_frame, raw_frame_data, ETH_MAC_ADDRS_LEN);
		memset(&processed_frame[ETH_TYPE_OFFSET_UNTAGGED], 0, ETH_QinQ_TAG_LEN);
		memcpy(&processed_frame[ETH_TYPE_OFFSET_SINGLE_TAG], &raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED], *frame_length - ETH_MAC_ADDRS_LEN);

		*frame_length += ETH_QinQ_TAG_LEN;
	} else if (raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED] == 0x88 && raw_frame_data[ETH_TYPE_OFFSET_UNTAGGED + 1] == 0xa8) {
		if ((raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG] == 0x81 && raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG + 1] == 0x00) &&
			(raw_frame_data[ETH_TYPE_OFFSET_DOUBLE_TAG] == 0x08 && raw_frame_data[ETH_TYPE_OFFSET_DOUBLE_TAG + 1] == 0x06)) {
			memcpy(processed_frame, raw_frame_data, *frame_length);
		} else if (raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG] == 0x08 && raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG + 1] == 0x06) {
			memcpy(processed_frame, raw_frame_data, ETH_MAC_ADDRS_LEN + ETH_QinQ_TAG_LEN);
			memset(&processed_frame[ETH_TYPE_OFFSET_SINGLE_TAG], 0, ETH_QinQ_TAG_LEN);
			memcpy(&processed_frame[ETH_TYPE_OFFSET_DOUBLE_TAG], &raw_frame_data[ETH_TYPE_OFFSET_SINGLE_TAG],
				   *frame_length - ETH_TYPE_OFFSET_SINGLE_TAG);
			*frame_length += ETH_QinQ_TAG_LEN;
		} else {
			*frame_length = 0;
			return;
		}
	} else {
		*frame_length = 0;
	}

	return;
}

static void set_device_data(device *device_data, unsigned char *processed_frame, int32_t *socket, device *device, pthread_t signal_thread) {
	time_t now;
	struct tm local_time;
	struct eth_header *eth = (struct eth_header *)processed_frame;
	struct arp_header *arp = (struct arp_header *)(processed_frame + sizeof(struct eth_header));

	memcpy(device_data->mac, eth->sour_addr, sizeof(eth->sour_addr));
	memcpy(device_data->ip, &arp->spa, sizeof(arp->spa));
	device_data->qinq_tag = ntohl(eth->qinq_tag);
	device_data->dot1q_tag = ntohl(eth->dot1q_tag);

	now = time(NULL);

	if (localtime_r(&now, &local_time) == NULL) {
		free(device);
		device = NULL;
		network_error(APP_ERR_LOCALTIME_R, socket, signal_thread);
		return;
	}

	atomic_store(&device_data->last_seen.hour, local_time.tm_hour);
	atomic_store(&device_data->last_seen.minutes, local_time.tm_min);
	atomic_store(&device_data->last_seen.seconds, local_time.tm_sec);

	return;
}

void *network_routine(void *args) {
	int32_t epoll_fd;
	int32_t socket_fd;
	int32_t timer_fd;
	hash_map map;
	pthread_t signal_thread = ((struct network_thread_args *)args)->signal_thread;
	struct epoll_event events[4];

	network_init(&socket_fd, (struct network_thread_args *)args, &map, &epoll_fd, &timer_fd);

	while (!atomic_load(&end_listen_loop)) {
		int32_t number_of_events = epoll_wait(epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == socket_fd) {
				unsigned char raw_frame_data[FRAME_BUFFER_SIZE]; // expect a standard-length frame (as defined by IEEE 802.3)
				unsigned char processed_frame[FRAME_BUFFER_SIZE];
				char control[1024];
				ssize_t frame_length = 0;
				device *device_data = malloc(sizeof(device) * 1);
				ui_message msg = {0};
				struct iovec iov = {.iov_base = raw_frame_data, .iov_len = sizeof(raw_frame_data)};
				struct msghdr control_msg = {
					.msg_name = NULL, .msg_namelen = 0, .msg_iov = &iov, .msg_iovlen = 1, .msg_control = control, .msg_controllen = sizeof(control)};

				memset(raw_frame_data, 0, sizeof(raw_frame_data));
				memset(processed_frame, 0, sizeof(processed_frame));
				memset(control, 0, sizeof(control));

				frame_length = recvmsg(socket_fd, &control_msg, 0);
				if (frame_length < 0) {
					free(device_data);
					device_data = NULL;
					break;
				}

				process_raw_arp_frame(raw_frame_data, processed_frame, &frame_length);
				if (frame_length <= 0 || (size_t)frame_length < sizeof(struct eth_header) + sizeof(struct arp_header)) {
					free(device_data);
					device_data = NULL;
					continue;
				}

				get_vlan_info(&control_msg, processed_frame, &socket_fd, signal_thread);

				set_device_data(device_data, processed_frame, &socket_fd, device_data, signal_thread);

				device *exitsing_device = hashmap_check_entry(&map, device_data->mac);

				if (exitsing_device != NULL) {
					atomic_store(&exitsing_device->last_seen.hour, device_data->last_seen.hour);
					atomic_store(&exitsing_device->last_seen.minutes, device_data->last_seen.minutes);
					atomic_store(&exitsing_device->last_seen.seconds, device_data->last_seen.seconds);

					msg.msg_type = UI_UPDATE_TABLE;
					msg.data = NULL;

					free(device_data);
					device_data = NULL;
				} else {
					if (hashmap_store_entry(&map, device_data) == -1) {
						network_error(APP_ERR_HASHMAP_SOTRE_ENTRY, &socket_fd, signal_thread);
					}

					msg.msg_type = UI_NEW_ENTRY;
					msg.data = device_data;
				}

				write(pipe_fd[1], &msg, sizeof(msg));
			} else if (events[i].data.fd == bind_update_fd) {
				uint64_t read_event;
				read(bind_update_fd, &read_event, sizeof(read_event)); // resets counter

				change_bind(&socket_fd, &epoll_fd, signal_thread);
			} else if (events[i].data.fd == shutdown_network_fd) {
				continue;
			}
		}
	}

	network_clean_up(&map, &socket_fd, &epoll_fd, &timer_fd);
	return NULL;
}
