#include "signal_handler.h"
#include "device.h"
#include "init.h"
#include "lifecycle.h"
#include "net/network_thread.h"
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

void *signal_routine(void *args) {
	(void)args;
	sigset_t mask;
	int32_t signal;
	uint32_t termination_reason;
	ui_message msg = {0};
	bool keep_running = true;

	signal_init(&mask);

	while (keep_running) {
		if (sigwait(&mask, &signal) != 0) {
			continue;
		}

		switch (signal) {
		case SIGWINCH:
			msg.msg_type = UI_RESIZE;
			msg.data = NULL;
			event_bus_publish(&msg);
			break;
		case SIGINT:
			termination_reason = SIGINT_END;
			keep_running = false;
			break;
		case SIGUSR1:
			termination_reason = SIGUSR1_END;
			keep_running = false;
			break;
		}
	}

	lifecycle_request_shutdown(termination_reason);
	network_request_shutdown();

	return NULL;
}
