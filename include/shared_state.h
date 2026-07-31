#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <net/if.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

extern atomic_bool end_main_loop;
extern atomic_bool end_listen_loop;
extern atomic_uint_fast32_t termination_reason;
extern int32_t shutdown_main_fd;
extern int32_t shutdown_network_fd;
extern int32_t pipe_fd[2];
extern struct if_nameindex binded_interface;
extern struct if_nameindex *interfaces;
extern int32_t number_of_records;
extern int32_t visible_records;
extern struct if_nameindex *interfaces;
extern int32_t bind_update_fd;
extern pthread_mutex_t binded_interface_mutex;

#endif
