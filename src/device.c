#include "device.h"
#include <stdint.h>

static uint64_t mac_to_u64(const uint8_t mac[6]) {
	return ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) | ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) | ((uint64_t)mac[4] << 8) |
		   ((uint64_t)mac[5]);
}

uint32_t hash_mac(const uint8_t mac[6]) {
	uint64_t mac_number = mac_to_u64(mac);

	mac_number ^= mac_number >> 33;
	mac_number *= 0xff51afd7ed558ccdULL;
	mac_number ^= mac_number >> 33;
	mac_number *= 0xc4ceb9fe1a85ec53ULL;
	mac_number ^= mac_number >> 33;

	return (uint32_t)mac_number;
}

bool hashmap_check_entry(const uint8_t *mac) {
	uint32_t index = hash_mac(mac) % map.size;
	bool entry_exists = false;

	if (map.table[index].device == NULL) {
	};
	return true;
}
