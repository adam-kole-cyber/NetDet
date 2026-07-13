#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H
#include <signal.h>

extern volatile sig_atomic_t end_main_loop;
extern volatile sig_atomic_t end_listen_loop;
extern volatile sig_atomic_t termination_reason;
extern int socket_fd;
typedef enum { PROGRAM_RUNNING, SIGINT_END, SIGUSR1_END } retval;

void sigint_handler(int arg);
void sigusr1_handler(int arg);

#endif
