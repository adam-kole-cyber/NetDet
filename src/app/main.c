#include "app/app_clean_up.h"
#include "app/app_context.h"
#include "app/app_init.h"
#include "app/lifecycle.h"
#include "common/error.h"
#include "core/device.h"
#include "net/network_thread.h"
#include "ui/popup_templates.h"
#include "ui/tui.h"
#include <ncurses.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	app_context variables = {0};
	struct epoll_event events[3];
	struct network_thread_args args;

	args.argc = argc;
	args.argv = argv;

	ncurses_init();
	main_init(&variables, &args);

	draw(&variables);
	while (!get_end_main_loop()) {
		int32_t number_of_events = epoll_wait(variables.epoll_fd, events, 3, -1);

		for (int32_t i = 0; i < number_of_events; i++) {
			if (events[i].data.fd == get_event_bus_fd()) {
				ui_message msg;
				read(get_event_bus_fd(), &msg, sizeof(ui_message));

				switch (msg.msg_type) {
					case UI_NEW_ENTRY:
						if (device_buffer_store_entry(variables.buffer.data, msg.data) == -1) {
							main_error(APP_ERR_SLIDINGWINDOWBUFFER_STORE_ENTRY);
						}

						variables.buffer.view.count++;
						draw(&variables);
						break;
					case UI_UPDATE_TABLE:

						break;
					case UI_RESIZE:
						resize_handler(&variables);
						draw(&variables);
						break;
					case UI_TIMER_TICK:
						if (variables.popup_window.is_active && variables.popup_window.popup_type == INSPECT_LIST) {
							draw(&variables);
						}
						break;
					default:
						break;
				}

			} else if (events[i].data.fd == STDIN_FILENO) {
				int32_t input = wgetch(variables.main_window.window);
				input_handler(input, &variables);

				draw(&variables);
			} else if (events[i].data.fd == get_shutdown_main_fd()) {
				continue;
			}
		}
	}

	main_clean_up(&variables);
	return 0;
}
