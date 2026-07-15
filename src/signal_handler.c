#include "signal_handler.h"
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

pthread_t signal_thread_id;

void *signal_routine(void *args) {
	(void)args;
	signal_thread_id = pthread_self();
	sigset_t mask;
	int signal;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGUSR1);

	if (sigwait(&mask, &signal) != 0) {
		return NULL;
	}

	switch (signal) {
	case SIGINT:
		termination_reason = SIGINT_END;
		break;

	case SIGUSR1:
		termination_reason = SIGUSR1_END;
		break;
	}

	uint64_t data = 1;
	atomic_store(&end_main_loop, true);
	atomic_store(&end_listen_loop, true);

	write(shutdown_fd, &data, sizeof(data));
	return NULL;
}
