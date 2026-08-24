#include "core/device.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static int32_t device_buffer_realloc(device_buffer *buffer) {
	uint32_t new_size = buffer->size << 1;
	device **tmp = realloc(buffer->items, buffer->size * sizeof(device *));
	if (tmp == NULL) {
		return -1;
	}

	for (uint32_t i = buffer->size; i < new_size; i++) {
		tmp[i] = NULL;
	}

	buffer->items = tmp;
	buffer->size = new_size;
	return 0;
}

int32_t device_buffer_store_entry(device_buffer *buffer, device *dev) {
	if ((buffer->count + 1) > (buffer->size * 0.8)) {
		if (device_buffer_realloc(buffer) == -1) {
			return -1;
		}
	}

	int32_t index = buffer->count;

	buffer->items[index] = dev;
	buffer->count++;
	return 0;
}

void device_raise_frame_count(device *device_to_update) {
	if (device_to_update == NULL) {
		return;
	}

	atomic_store(&device_to_update->total_frames, atomic_load(&device_to_update->total_frames) + 1);

	return;
}
