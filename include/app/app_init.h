#ifndef APP_INIT_H
#define APP_INIT_H

#include "app_context.h"
#include "net/network_thread.h"
#include "ui/tui.h"
#include <stdint.h>

void main_init(app_context *variables, struct network_thread_args *args);

#endif
