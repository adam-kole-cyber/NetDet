#ifndef NETWORK_H
#define NETWORK_H
#include <pthread.h>

extern pthread_t main_thread_id;

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
void network_init(int *socket_fd);

#endif
