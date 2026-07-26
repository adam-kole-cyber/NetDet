#include "signal_handler.h"
#include "device.h"
#include "shared_state.h"
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
	ui_message msg;
	bool keep_running = true;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);

	while (keep_running) {
		if (sigwait(&mask, &signal) != 0) {
			continue;
		}

		switch (signal) {
		case SIGWINCH:
			msg.msg_type = UI_RESIZE;
			msg.data = NULL;
			write(pipe_fd[1], &msg, sizeof(msg));
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

	uint64_t data = 1;
	atomic_store(&end_main_loop, true);
	atomic_store(&end_listen_loop, true);

	write(shutdown_network_fd, &data, sizeof(data));
	write(shutdown_main_fd, &data, sizeof(data));
	return NULL;
}
