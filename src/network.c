#include "network.h"
#include "device.h"
#include "error.h"
#include "init.h"
#include "shared_state.h"
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
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

	device_data->last_seen.hour = local_time.tm_hour;
	device_data->last_seen.minutes = local_time.tm_min;
	device_data->last_seen.seconds = local_time.tm_sec;

	return;
}

void *network_routine(void *args) {
	int32_t epoll_fd;
	int32_t socket_fd;
	hash_map map;
	pthread_t signal_thread = ((struct network_thread_args *)args)->signal_thread;
	struct epoll_event events[2];

	network_init(&socket_fd, (struct network_thread_args *)args, &map, &epoll_fd);

	while (!atomic_load(&end_listen_loop)) {
		int32_t number_of_events = epoll_wait(epoll_fd, events, 2, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == socket_fd) {
				unsigned char raw_frame_data[FRAME_BUFFER_SIZE]; // expect a standard-length frame (as defined by IEEE 802.3)
				unsigned char processed_frame[FRAME_BUFFER_SIZE];
				ssize_t frame_length = 0;
				device *device_data = malloc(sizeof(device) * 1);
				ui_message msg;

				memset(raw_frame_data, 0, sizeof(raw_frame_data));
				memset(processed_frame, 0, sizeof(processed_frame));

				frame_length = recvfrom(socket_fd, raw_frame_data, sizeof(raw_frame_data), 0, NULL, NULL);
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

				set_device_data(device_data, processed_frame, &socket_fd, device_data, signal_thread);

				pthread_mutex_lock(&device_data_structures_mutex);
				device *exitsing_device = hashmap_check_entry(&map, device_data->mac);
				if (exitsing_device != NULL) {
					exitsing_device->last_seen.hour = device_data->last_seen.hour;
					exitsing_device->last_seen.minutes = device_data->last_seen.minutes;
					exitsing_device->last_seen.seconds = device_data->last_seen.seconds;

					msg.msg_type = UI_UPDATE_TABLE;
					msg.data = NULL;

					free(device_data);
					device_data = NULL;
				} else {
					if (hashmap_store_entry(&map, device_data) == -1) {
						network_error(APP_ERR_HASHMAP_SOTRE_ENTRY, &socket_fd, signal_thread);
						pthread_mutex_unlock(&device_data_structures_mutex);
					}

					msg.msg_type = UI_NEW_ENTRY;
					msg.data = device_data;
				}
				write(pipe_fd[1], &msg, sizeof(msg));
				pthread_mutex_unlock(&device_data_structures_mutex);

			} else if (events[i].data.fd == shutdown_network_fd) {
				continue;
			}
		}
	}

	free(map.table);
	map.table = NULL;

	close(epoll_fd);
	close(socket_fd);
	close(shutdown_network_fd);
	return NULL;
}
