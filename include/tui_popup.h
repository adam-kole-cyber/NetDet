#ifndef TUI_POPUP_H
#define TUI_POPUP_H

#include "core/device.h"
#include "tui.h"
#include "ui/popup_templates.h"
#include <pthread.h>
#include <stdint.h>

void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_tpye, device *action_device);

#endif
