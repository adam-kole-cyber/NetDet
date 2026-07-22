#include "error.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static error_code error_app;
static int32_t errno_val_app = 0;

static const char *error_code_to_text(error_code err) {
	switch (err) {
	case APP_ERR_NONE:
		return "no error";
	case APP_ERR_BIND:
		return "bind() failed";
	case APP_ERR_SOCKET:
		return "socket() failed";
	case APP_ERR_IF_NAMETOINDEX:
		return "if_nametoindex() failed";
	case APP_ERR_LOCALTIME_R:
		return "localtime_r() failed";
	case APP_ERR_HASHMAP_SOTRE_ENTRY:
		return "hashmap_store_entry() failed";
	case APP_ERR_SLIDINGWINDOWBUFFER_STORE_ENTRY:
		return "slidingwindowbuffer_store_entry() failed";
	}
	return "unknown error";
}

void set_error(error_code error, int32_t errno_val) {
	error_app = error;
	errno_val_app = errno_val;
	return;
}

void get_error(void) {
	if (error_app == APP_ERR_NONE) {
		return;
	}

	errno = errno_val_app;
	perror(error_code_to_text(error_app));

	return;
}

void network_error(error_code error, int32_t *socket) {
	set_error(error, errno);
	close(*socket);
	pthread_kill(signal_thread, SIGUSR1);
	pthread_exit(NULL);
	return;
}
