#ifndef CORE_DEVICE_H
#define CORE_DEVICE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_INITIAL_SIZE 128
#define RATE_HISTORY_SIZE 60

typedef struct {
	atomic_uint_least32_t data[RATE_HISTORY_SIZE];
	int32_t head;
	int32_t count;
} rate_history;

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
	atomic_uint_least64_t previous_frames;
	atomic_uint_least64_t total_frames;
	time_struct last_seen;
	rate_history graph;
} device;

typedef struct {
	device **items; // array of pointers to devices
	uint32_t size;	// size of whole buffer
	uint32_t count;
} device_buffer;

typedef struct {
	uint8_t mac[6];
	device *device;
} hash_entry;

typedef struct {
	hash_entry *table;
	uint32_t size;
	uint32_t count;
} hash_map;

enum ui_message_type { UI_NEW_ENTRY = 0, UI_UPDATE_TABLE, UI_RESIZE, UI_TIMER_TICK };
typedef struct {
	enum ui_message_type msg_type;
	device *data;
} ui_message;

int32_t device_buffer_store_entry(device_buffer *buffer, device *dev);
void device_raise_frame_count(device *device_to_update);

#endif
