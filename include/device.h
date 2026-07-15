#ifndef DEVICE_H
#define DEVICE_H

#include <pthread.h>

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

	pthread_mutex_t mutex;
} ring_buffer;

typedef struct {
	unsigned char mac[6];
	device *device;
} hash_entry;

typedef struct {
	hash_entry *table;
	size_t size;
	pthread_mutex_t mutex;
} HashMap;

#endif
