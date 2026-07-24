#ifndef DEVICE_H
#define DEVICE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_INITIAL_SIZE 128

typedef struct {
	uint8_t hour;
	uint8_t minutes;
	uint8_t seconds;
} time_struct;

typedef struct {
	uint8_t mac[6];
	uint8_t ip[4];
	uint32_t qinq_tag;
	uint32_t dot1q_tag;
	time_struct last_seen;
} device;

typedef struct {
	device **items;			// array of pointers to devices
	uint32_t count;			// number of curently stored devices
	uint32_t display_row;	// stores a number that indicates how many rows of records can be currently displayed
	uint32_t display_limit; // stores the number that indicates how many records can be safely displayed (it will be either size or display_row)
	uint32_t head;			// index of the device to start displaying from
	uint32_t size;			// size of whole buffer
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

extern sliding_window_buffer buffer;

device *hashmap_check_entry(hash_map *map, const uint8_t *mac);
int32_t hashmap_store_entry(hash_map *map, device *dev);
int32_t slidingwindowbuffer_store_entry(device *dev);

#endif
