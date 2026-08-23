#ifndef PLATFORM_SIGNAL_HANDLER_H
#define PLATFORM_SIGNAL_HANDLER_H

#include <bits/types/sigset_t.h>
#include <stdatomic.h>
#include <stdint.h>

typedef enum { PROGRAM_RUNNING, SIGINT_END, SIGUSR1_END } retval;

void signal_init(sigset_t *mask);
void *signal_routine(void *args);

#endif
