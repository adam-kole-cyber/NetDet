#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "tui_popup.h"
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
extern struct if_nameindex *popup_list;
extern int32_t bind_update_fd;
extern pthread_mutex_t binded_interface_mutex;
extern const inspect_field_t popup_inspect[6];
extern popup_descriptor popup_descriptors[POPUP_TYPE_COUNT];
extern pthread_t signal_thread;

#endif
