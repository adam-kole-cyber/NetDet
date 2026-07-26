#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <pthread.h>
#include <stdatomic.h>

extern atomic_bool end_main_loop;
extern atomic_bool end_listen_loop;
extern atomic_uint_fast32_t termination_reason;
extern int32_t shutdown_main_fd;
extern int32_t shutdown_network_fd;
extern int32_t pipe_fd[2];
extern pthread_mutex_t device_data_structures_mutex;

#endif
