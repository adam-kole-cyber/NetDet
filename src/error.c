#include "error.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static error_code error_app;
static int errno_val_app = 0;

static const char *error_code_to_text(error_code err) {
	switch (err) {
	case APP_ERR_NONE:
		return "no error";
	case APP_ERR_BIND:
		return "bind() failed";
	case APP_ERR_SOCKET:
		return "socket() failed";
	case APP_ERR_SETSOCKOPT:
		return "setsockopt() failed";
	case APP_ERR_IF_NAMETOINDEX:
		return "if_nametoindex() failed";
	}
	return "unknown error";
}

void set_error(error_code error, int errno_val) {
	error_app = error;
	errno_val_app = errno_val;
}

void get_error(void) {
	if (error_app == APP_ERR_NONE) {
		return;
	}

	errno = errno_val_app;
	perror(error_code_to_text(error_app));
}

void network_error(error_code error, int *socket) {
	set_error(error, errno);
	close(*socket);
	pthread_kill(main_thread_id, SIGUSR1);
	pthread_exit(NULL);
	return;
}
