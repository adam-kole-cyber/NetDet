#include "core/hash_map.h"
#include "common/error.h"
#include "core/device.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t mac_to_u64(const uint8_t mac[6]) {
	return ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) | ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) |
		   ((uint64_t)mac[4] << 8) | ((uint64_t)mac[5]);
}

static uint32_t hash_mac(const uint8_t mac[6]) {
	uint64_t mac_number = mac_to_u64(mac);

	mac_number ^= mac_number >> 33;
	mac_number *= 0xff51afd7ed558ccdULL;
	mac_number ^= mac_number >> 33;
	mac_number *= 0xc4ceb9fe1a85ec53ULL;
	mac_number ^= mac_number >> 33;

	return (uint32_t)mac_number;
}

static void store_entry(hash_entry *map, hash_entry entry_to_save, uint32_t map_size) {
	size_t index = hash_mac(entry_to_save.mac) % map_size;

	while (map[index].device != NULL) {
		index = (index + 1) % map_size;
	}

	memcpy(map[index].mac, entry_to_save.mac, 6);
	map[index].device = entry_to_save.device;
	return;
}

static int32_t hashmap_realloc(hash_map *map) {
	uint32_t new_size = map->size << 1;

	hash_entry *tmp = calloc(new_size, sizeof(hash_entry));
	if (tmp == NULL) {
		return -1;
	}

	for (uint32_t i = 0; i < map->size; i++) {
		if (map->table[i].device == NULL) {
			continue;
		} else {
			store_entry(tmp, map->table[i], new_size);
		}
	}

	free(map->table);

	map->table = tmp;
	map->size = new_size;

	return 0;
}

device *hashmap_check_entry(hash_map *map, const uint8_t *mac) {
	uint32_t index = hash_mac(mac) % map->size;
	uint32_t start_index = index;

	while (map->table[index].device != NULL) {
		if (memcmp(map->table[index].mac, mac, 6) == 0) {
			return map->table[index].device;
		}

		index = (index + 1) % map->size;

		if (index == start_index) {
			break;
		}
	}

	return NULL;
}

int32_t hashmap_store_entry(hash_map *map, device *dev) {
	if ((map->count + 1) > (map->size * 0.8)) {
		if (hashmap_realloc(map) == -1) {
			return -1;
		}
	}

	hash_entry entry;
	entry.device = dev;
	memcpy(entry.mac, dev->mac, 6);

	store_entry(map->table, entry, map->size);
	map->count++;
	return 0;
}

bool device_registry_upsert(hash_map *map, device *incoming, int32_t socket_fd) {
	device *existing_device = hashmap_check_entry(map, incoming->mac);

	if (existing_device != NULL) {
		atomic_store(&existing_device->last_seen.hour, incoming->last_seen.hour);
		atomic_store(&existing_device->last_seen.minutes, incoming->last_seen.minutes);
		atomic_store(&existing_device->last_seen.seconds, incoming->last_seen.seconds);

		return true;
	} else {
		if (hashmap_store_entry(map, incoming) == -1) {
			network_error(APP_ERR_HASHMAP_STORE_ENTRY, socket_fd);
		}

		atomic_store(&incoming->previous_frames, 0);
		atomic_store(&incoming->total_frames,
					 1); // 1 because this frame, through which we discovered this device, also counts

		incoming->graph.head = 0;
		incoming->graph.count = 0;

		for (uint32_t i = 0; i < RATE_HISTORY_SIZE; i++) {
			atomic_store(&incoming->graph.data[i], 0);
		}

		return false;
	}
}
