#include "net/network_thread.h"
#include "app/lifecycle.h"
#include "common/error.h"
#include "core/device.h"
#include "net/frame_parser.h"
#include "net/raw_socket.h"
#include "platform/epoll_utils.h"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

static atomic_bool end_listen_loop = false;
static int32_t shutdown_network_fd;

static void handle_incoming_frame(int32_t socket_fd, hash_map *map) {
	unsigned char raw_frame_data[FRAME_BUFFER_SIZE]; // expect a standard-length frame (as defined by IEEE 802.3)
	unsigned char processed_frame[FRAME_BUFFER_SIZE];
	char control[1024];
	ui_message msg = {0};
	ssize_t frame_length = 0;
	device *device_data = malloc(sizeof(device) * 1);
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
		return;
	}

	raise_frame_count(hashmap_check_entry(map, &raw_frame_data[6])); // TODO consider increasing the frame rate and saving the device

	process_raw_arp_frame(raw_frame_data, processed_frame, &frame_length);
	if (frame_length <= 0 || (size_t)frame_length < sizeof(struct eth_header) + sizeof(struct arp_header)) {
		free(device_data);
		device_data = NULL;
		return;
	}

	get_vlan_info(&control_msg, processed_frame, &socket_fd);
	set_device_data(device_data, processed_frame, &socket_fd);

	if (device_registry_upsert(map, device_data, socket_fd)) {
		msg.msg_type = UI_UPDATE_TABLE;
		msg.data = NULL; // this can be used when optimizing TUI
	} else {
		msg.msg_type = UI_NEW_ENTRY;
		msg.data = device_data;
	}

	event_bus_publish(&msg);

	return;
}

static void handle_bind_update(int32_t *socket_fd, int32_t *epoll_fd) {
	uint64_t read_event;
	read(get_bind_update_fd(), &read_event, sizeof(read_event));

	change_bind(socket_fd, epoll_fd);

	return;
}

static void handle_timer_tick(int32_t socket_fd, int32_t timer_fd, hash_map *map) {
	uint64_t timer_ticks = 0;
	ui_message msg = {0};
	ssize_t return_val = read(timer_fd, &timer_ticks, sizeof(timer_ticks));

	if (return_val != sizeof(timer_ticks)) {
		network_error(APP_ERR_TIMER, socket_fd);
	}

	for (uint32_t i = 0; i < map->size; i++) {
		if (map->table[i].device == NULL) {
			continue;
		}

		device *device_to_update = map->table[i].device;
		uint32_t rate = atomic_load(&device_to_update->total_frames) - atomic_load(&device_to_update->previous_frames);

		atomic_store(&device_to_update->graph.data[device_to_update->graph.head], rate);
		atomic_store(&device_to_update->previous_frames, atomic_load(&device_to_update->total_frames));

		device_to_update->graph.head = (device_to_update->graph.head + 1) % RATE_HISTORY_SIZE;
	}

	msg.msg_type = UI_UPDATE_TABLE;
	msg.data = NULL;
	event_bus_publish(&msg);

	return;
}

void network_init(int32_t *socket_fd, int32_t *epoll_fd, int32_t *timer_fd, hash_map *map, struct network_thread_args *args) {
	struct itimerspec timer = {.it_value = {.tv_sec = 1, .tv_nsec = 0}, .it_interval = {.tv_sec = 1, .tv_nsec = 0}};

	*socket_fd = raw_socket_create();

	shutdown_network_fd = eventfd(0, 0);

	*timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	timerfd_settime(*timer_fd, 0, &timer, NULL);

	*epoll_fd = epoll_create1(0);
	epoll_register(*epoll_fd, shutdown_network_fd);
	epoll_register(*epoll_fd, *timer_fd);

	socket_init(*socket_fd, *epoll_fd, (args->argc > 1) ? args->argv[1] : NULL);

	map->size = BUFFER_INITIAL_SIZE;
	map->count = 0;
	map->table = calloc(map->size, sizeof(hash_entry));

	if (map->table == NULL) {
		network_error(APP_ERR_CALLOC, *socket_fd);
		return;
	}

	return;
}

void *network_routine(void *args) {
	int32_t socket_fd;
	int32_t epoll_fd;
	int32_t timer_fd;
	hash_map map;
	struct epoll_event events[4];

	network_init(&socket_fd, &epoll_fd, &timer_fd, &map, (struct network_thread_args *)args);

	while (!atomic_load(&end_listen_loop)) {
		int32_t number_of_events = epoll_wait(epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == socket_fd) {
				handle_incoming_frame(socket_fd, &map);
			} else if (events[i].data.fd == get_bind_update_fd()) {
				handle_bind_update(&socket_fd, &epoll_fd);
			} else if (events[i].data.fd == timer_fd) {
				handle_timer_tick(socket_fd, timer_fd, &map);
			} else if (events[i].data.fd == shutdown_network_fd) {
				continue;
			}
		}
	}

	network_clean_up(&socket_fd, &epoll_fd, &timer_fd, &map);
	return NULL;
}

void network_request_shutdown(void) {
	uint64_t data = 1;

	atomic_store(&end_listen_loop, true);
	write(shutdown_network_fd, &data, sizeof(data));

	return;
}

void network_clean_up(int32_t *socket_fd, int32_t *epoll_fd, int32_t *timer_fd, hash_map *map) {
	free(map->table);
	map->table = NULL;

	socket_cleanup();

	close(*socket_fd);
	close(*epoll_fd);
	close(*timer_fd);
	close(shutdown_network_fd);
}
