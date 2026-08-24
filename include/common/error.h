#ifndef COMMON_ERROR_H
#define COMMON_ERROR_H

#include <stdint.h>

typedef enum {
	APP_ERR_ANCILLARY_DATA,
	APP_ERR_BIND,
	APP_ERR_CALLOC,
	APP_ERR_HASHMAP_STORE_ENTRY,
	APP_ERR_IF_NAMEINDEX,
	APP_ERR_IF_NAMETOINDEX,
	APP_ERR_LOCALTIME_R,
	APP_ERR_NONE,
	APP_ERR_SETSOCKOPT,
	APP_ERR_SLIDINGWINDOWBUFFER_STORE_ENTRY,
	APP_ERR_SOCKET,
	APP_ERR_TIMER
} error_code;

void set_error(error_code error, int32_t errno_val);
void get_error(void);
void network_error(error_code error, int32_t socket);
void main_error(error_code error);

#endif
