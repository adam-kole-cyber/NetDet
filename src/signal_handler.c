#include "signal_handler.h"

void sigint_handler(int arg) {
	(void)arg;
	end_main_loop = 1;
	termination_reason = SIGINT_END;
}

void sigusr1_handler(int arg) {
	(void)arg;
	end_main_loop = 1;
	termination_reason = SIGUSR1_END;
}
