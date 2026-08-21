#ifndef NET_FRAMEPARSER_H
#define NET_FRAMEPARSER_H

#include "device.h"
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

void get_vlan_info(struct msghdr *control_msg, unsigned char *processed_frame, int32_t *socket_fd);
void process_raw_arp_frame(unsigned char *raw_frame_data, unsigned char *processed_frame, ssize_t *frame_length);
void set_device_data(device *device_data, unsigned char *processed_frame, int32_t *socket, device *device);

#endif
