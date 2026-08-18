#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "device.h"
#include "popup_templates.h"
#include "tui.h"
#include <pthread.h>
#include <stdint.h>

typedef struct app_context {
	int32_t epoll_fd;

	pthread_t network_thread;

	sliding_window_buffer buffer;
	window_data main_window;
	popup_window_data popup_window;
} app_context;

#endif
