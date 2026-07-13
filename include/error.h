#ifndef ERROR_H
#define ERROR_H
#include <pthread.h>

extern pthread_t main_thread_id;
typedef enum { APP_ERR_NONE = 0, APP_ERR_BIND, APP_ERR_SOCKET, APP_ERR_SETSOCKOPT, APP_ERR_IF_NAMETOINDEX } error_code;

void set_error(error_code error, int errno_val);
void get_error(void);
void network_error(error_code error, int *socket);

#endif
