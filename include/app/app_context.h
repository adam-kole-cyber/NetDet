#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "ui/popup_templates.h"
#include "ui/tui.h"
#include "ui/tui_app.h"
#include <bits/pthreadtypes.h>
#include <stdint.h>

typedef struct app_context {
	int32_t epoll_fd;

	pthread_t network_thread;

	device_table_view buffer;
	window_data main_window;
	popup_window_data popup_window;
} app_context;

#endif
