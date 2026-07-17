#ifndef DEVICE_H
#define DEVICE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	unsigned char hour;
	unsigned char minutes;
	unsigned char seconds;
} time_struct;

typedef struct {
	unsigned char mac[6];
	unsigned char ip[4];
	unsigned char qinq_dotq_tags[3];
	time_struct last_seen;
} device;

typedef struct {
	device **items;
	unsigned int capacity;
	unsigned int head;
	unsigned int count;
	unsigned int display_limit;
	pthread_mutex_t mutex;
} sliding_window_buffer;

typedef struct {
	unsigned char mac[6];
	device *device;
} hash_entry;

typedef struct {
	hash_entry *table;
	size_t size;
	pthread_mutex_t mutex;
} hash_map;

extern hash_map map;
extern sliding_window_buffer buffer;

uint32_t hash_mac(const uint8_t mac[6]);
device *hashmap_check_entry(const uint8_t mac[6]);
int hashmap_store_entry(device *dev);
int slidingwindowbuffer_store_entry(device *dev);

#endif
