#include "net/frame_parser.h"
#include "core/device.h"
#include "error.h"
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void get_vlan_info(struct msghdr *control_msg, unsigned char *processed_frame, int32_t *socket_fd) {
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
					network_error(APP_ERR_ANCILLARY_DATA, *socket_fd);
					return;
				}

			} else { // frame was not tagged continue
				continue;
			}
#else
			// rx offload off or rebuild against newer Linux headers
			network_error(APP_ERR_ANCILLARY_DATA, *socket_fd);
#endif
		}
	}

	return;
}

void process_raw_arp_frame(unsigned char *raw_frame_data, unsigned char *processed_frame, ssize_t *frame_length) {
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

void set_device_data(device *device_data, unsigned char *processed_frame, int32_t *socket) {
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
		free(device_data);
		device_data = NULL;
		network_error(APP_ERR_LOCALTIME_R, *socket);
		return;
	}

	atomic_store(&device_data->last_seen.hour, local_time.tm_hour);
	atomic_store(&device_data->last_seen.minutes, local_time.tm_min);
	atomic_store(&device_data->last_seen.seconds, local_time.tm_sec);

	return;
}
