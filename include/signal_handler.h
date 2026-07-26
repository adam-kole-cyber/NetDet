#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdatomic.h>
#include <stdint.h>

typedef enum { PROGRAM_RUNNING, SIGINT_END, SIGUSR1_END } retval;

void *signal_routine(void *args);

#endif
