#include "device.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

device *hashmap_check_entry(const uint8_t *mac) {
	uint32_t index = hash_mac(mac) % map.size;
	uint32_t start_index = index;

	while (map.table[index].device != NULL) {
		if (memcmp(map.table[index].mac, mac, 6) == 0) {
			return map.table[index].device;
		}

		index = (index + 1) % map.size;

		if (index == start_index) {
			break;
		}
	}

	return NULL;
}
