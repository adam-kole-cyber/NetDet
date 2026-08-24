#ifndef CORE_HASHMAP_H
#define CORE_HASHMAP_H

#include "core/device.h"
#include <stdint.h>

device *hashmap_check_entry(hash_map *map, const uint8_t *mac);
int32_t hashmap_store_entry(hash_map *map, device *dev);
bool device_registry_upsert(hash_map *map, device *incoming, int32_t socket_fd);

#endif
