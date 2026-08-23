#include "lifecycle.h"
#include "platform/epoll_utils.h"
#include "platform/signal_handler.h"
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <unistd.h>

static atomic_bool end_main_loop;
static atomic_uint_fast32_t termination_reason;
static int32_t shutdown_main_fd;
static pthread_t signal_thread;
static int32_t pipe_fd[2];

void lifecycle_init(int32_t epoll_fd) {
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	pthread_create(&signal_thread, NULL, signal_routine, NULL);

	pipe(pipe_fd);
	shutdown_main_fd = eventfd(0, 0);

	epoll_register(epoll_fd, shutdown_main_fd);
	epoll_register(epoll_fd, pipe_fd[0]);
	return;
}

void lifecycle_cleanup(void) {
	pthread_join(signal_thread, NULL);

	close(pipe_fd[0]);
	close(pipe_fd[1]);
	close(shutdown_main_fd);
	return;
}

void lifecycle_request_shutdown(uint32_t reason) {
	uint64_t data = 1;

	atomic_store(&termination_reason, reason);
	atomic_store(&end_main_loop, true);
	write(shutdown_main_fd, &data, sizeof(data));

	return;
}

bool get_end_main_loop(void) { return atomic_load(&end_main_loop); }
int32_t get_shutdown_main_fd(void) { return shutdown_main_fd; }

void lifecycle_notify_fatal_error(void) {
	pthread_kill(signal_thread, SIGUSR1);
	return;
}

void event_bus_publish(const ui_message *msg) {
	write(pipe_fd[1], msg, sizeof(*msg));
	return;
}

int32_t get_event_bus_fd(void) { return pipe_fd[0]; }
