#ifndef DEVICE_H
#define DEVICE_H

#include "scroll_view.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_INITIAL_SIZE 128

typedef struct {
	atomic_uint_least8_t hour;
	atomic_uint_least8_t minutes;
	atomic_uint_least8_t seconds;
} time_struct;

typedef struct {
	uint8_t mac[6];
	uint8_t ip[4];
	uint32_t qinq_tag;
	uint32_t dot1q_tag;
	time_struct last_seen;
} device;

typedef struct {
	device **items; // array of pointers to devices
	uint32_t size;	// size of whole buffer
	scroll_view view;
} sliding_window_buffer;

typedef struct {
	uint8_t mac[6];
	device *device;
} hash_entry;

typedef struct {
	hash_entry *table;
	uint32_t size;
	uint32_t count;
} hash_map;

enum ui_message_type { UI_NEW_ENTRY = 0, UI_UPDATE_TABLE, UI_RESIZE };
typedef struct {
	enum ui_message_type msg_type;
	device *data;
} ui_message;

device *hashmap_check_entry(hash_map *map, const uint8_t *mac);
int32_t hashmap_store_entry(hash_map *map, device *dev);
int32_t slidingwindowbuffer_store_entry(sliding_window_buffer *buffer, device *dev);

#endif
