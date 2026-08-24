#ifndef APP_LIFECYCLE_H
#define APP_LIFECYCLE_H

#include "core/device.h"
#include <stdbool.h>
#include <stdint.h>

void lifecycle_init(int32_t epoll_fd);
void lifecycle_cleanup(void);
void lifecycle_request_shutdown(uint32_t reason);
bool get_end_main_loop(void);
int32_t get_shutdown_main_fd(void);
void lifecycle_notify_fatal_error(void);
void event_bus_publish(const ui_message *msg);
int32_t get_event_bus_fd(void);

#endif
