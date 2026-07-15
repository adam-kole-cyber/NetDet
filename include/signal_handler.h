#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdatomic.h>

extern atomic_bool end_main_loop;
extern atomic_bool end_listen_loop;
extern atomic_uint_fast32_t termination_reason;
extern int shutdown_fd;
typedef enum { PROGRAM_RUNNING, SIGINT_END, SIGUSR1_END } retval;

void *signal_routine(void *args);

#endif
