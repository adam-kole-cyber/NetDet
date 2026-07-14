#include "signal_handler.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

	sigwait(&mask, &signal);

	uint64_t data = 1;
	end_main_loop = true;
	end_listen_loop = true;

	write(shutdown_fd, &data, sizeof(data));
	return NULL;
}
