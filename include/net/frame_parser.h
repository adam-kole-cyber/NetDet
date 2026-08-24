#ifndef NET_FRAMEPARSER_H
#define NET_FRAMEPARSER_H

#include "core/device.h"
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

#define ETH_MAC_ADDRS_LEN 12	  // dst mac (6B) + src mac (6B)
#define ETH_QinQ_TAG_LEN 4		  // in bytes
#define ETH_QinQ_DOT1Q_TAGS_LEN 8 // in byte
#define FRAME_BUFFER_SIZE 2048
#define ETH_TYPE_OFFSET_UNTAGGED 12	  // EtherType/TPID (no tag)
#define ETH_TYPE_OFFSET_SINGLE_TAG 16 // EtherType/TPID after (802.1Q) tag
#define ETH_TYPE_OFFSET_DOUBLE_TAG 20 // EtherType after (802.1ad + 802.1Q) tags

struct eth_header {
	uint8_t dest_addr[6];
	uint8_t sour_addr[6];
	uint32_t qinq_tag;	// IEEE 802.1ad - optional field
	uint32_t dot1q_tag; // IEEE 802.1Q - optional field
	uint16_t ether_type;
} __attribute__((packed));

struct arp_header {
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t oper;
	uint8_t sha[6];
	uint32_t spa;
	uint8_t tha[6];
	uint32_t tpa;
} __attribute__((packed));

void get_vlan_info(struct msghdr *control_msg, unsigned char *processed_frame, int32_t *socket_fd);
void process_raw_arp_frame(unsigned char *raw_frame_data, unsigned char *processed_frame, ssize_t *frame_length);
void set_device_data(device *device_data, unsigned char *processed_frame, int32_t *socket);

#endif
