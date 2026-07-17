#ifndef NETWORK_H
#define NETWORK_H

#include "device.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

#define ETH_MAC_ADDRS_LEN 12	  // dst mac (6B) + src mac (6B)
#define ETH_QinQ_TAG_LEN 4		  // in bytes
#define ETH_QinQ_DOT1Q_TAGS_LEN 8 // in bytes

extern pthread_t signal_thread_id;
extern int shutdown_fd;
extern sliding_window_buffer buffer;
extern hash_map map;
extern pthread_mutex_t device_data_structures_mutex;

struct network_thread_args {
	int argc;
	char **argv;
};

struct eth_header {
	unsigned char dest_addr[6];
	unsigned char sour_addr[6];
	unsigned int qinq_tag;	// IEEE 802.1ad - optional field
	unsigned int dot1q_tag; // IEEE 802.1Q - optional field
	unsigned short ether_type;
} __attribute__((packed));

struct arp_header {
	unsigned short htype;
	unsigned short ptype;
	unsigned char hlen;
	unsigned char plen;
	unsigned short oper;
	unsigned char sha[6];
	unsigned int spa;
	unsigned char tha[6];
	unsigned int tpa;
} __attribute__((packed));

void *network_routine(void *args);

#endif
