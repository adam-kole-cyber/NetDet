#include "device.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void hashmap_rehash(void) {
	for (unsigned int i = 0; i < buffer.count; i++) {
		hashmap_store_entry(buffer.items[i]);
	}

	map.count = buffer.count;
	return;
}

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

int hashmap_store_entry(device *dev) {
	if ((map.count + 1) > (map.size * 0.8)) {
		if (hashmap_realloc() == -1) {
			return -1;
		}
	}

	size_t index = hash_mac(dev->mac) % map.size;

	while (map.table[index].device != NULL) {
		index = (index + 1) % map.size;
	}

	memcpy(map.table[index].mac, dev->mac, 6);
	map.table[index].device = dev;
	map.count++;
	return 0;
}

int slidingwindowbuffer_store_entry(device *dev) {
	int index = buffer.count;

	buffer.items[index] = dev;
	buffer.count++;
	return 0;
}

int hashmap_realloc(void) {
	size_t new_size = map.size << 1;
	hash_entry *tmp;

	free(map.table);
	map.table = NULL;
	map.count = 0;

	tmp = calloc(new_size, sizeof(hash_entry));
	if (tmp == NULL) {
		return -1;
	}

	map.table = tmp;
	map.size = new_size;

	hashmap_rehash();
	return 0;
}
