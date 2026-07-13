#include "signal_handler.h"
#include <stdbool.h>
#include <sys/socket.h>

void sigint_handler(int arg) {
	(void)arg;
	end_listen_loop = true;
	end_main_loop = true;
	termination_reason = SIGINT_END;
}

void sigusr1_handler(int arg) {
	(void)arg;
	end_listen_loop = true;
	end_main_loop = true;
	termination_reason = SIGUSR1_END;
}
