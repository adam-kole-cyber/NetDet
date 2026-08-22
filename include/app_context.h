#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "tui.h"
#include "tui_app.h"
#include "ui/popup_templates.h"
#include <pthread.h>
#include <stdint.h>

typedef struct app_context {
	int32_t epoll_fd;

	pthread_t network_thread;

	device_table_view buffer;
	window_data main_window;
	popup_window_data popup_window;
} app_context;

#endif
