#include "common/error.h"
#include "lifecycle.h"
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static atomic_bool error_set = false;
static error_code error_app = APP_ERR_NONE;
static int32_t errno_val_app = 0;

static const char *error_code_to_text(error_code err) {
	switch (err) {
	case APP_ERR_ANCILLARY_DATA:
		return "Ancillary data are not available. Try disabling RX VLAN offload for that interface, or rebuild against newer Linux headers.";
	case APP_ERR_BIND:
		return "bind() failed";
	case APP_ERR_CALLOC:
		return "calloc() failed";
	case APP_ERR_HASHMAP_STORE_ENTRY:
		return "hashmap_store_entry() failed";
	case APP_ERR_IF_NAMEINDEX:
		return "if_nameindex() failed";
	case APP_ERR_IF_NAMETOINDEX:
		return "if_nametoindex() failed";
	case APP_ERR_LOCALTIME_R:
		return "localtime_r() failed";
	case APP_ERR_NONE:
		return "no error";
	case APP_ERR_SETSOCKOPT:
		return "setsockopt() failed";
	case APP_ERR_SLIDINGWINDOWBUFFER_STORE_ENTRY:
		return "slidingwindowbuffer_store_entry() failed";
	case APP_ERR_SOCKET:
		return "socket() failed";
	case APP_ERR_TIMER:
		return "timer() failed";
	}
	return "unknown error";
}

void set_error(error_code error, int32_t errno_val) {
	bool expected = false;

	if (atomic_compare_exchange_strong(&error_set, &expected, true)) {
		error_app = error;
		errno_val_app = errno_val;
	}

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

void network_error(error_code error, int32_t socket) {
	set_error(error, errno);
	close(socket);
	lifecycle_notify_fatal_error();
	pthread_exit(NULL);
	return;
}

void main_error(error_code error) {
	set_error(error, errno);
	lifecycle_notify_fatal_error();
	return;
}
