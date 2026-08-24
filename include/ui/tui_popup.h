#ifndef UI_TUI_POPUP_H
#define UI_TUI_POPUP_H

#include "core/device.h"
#include "tui.h"
#include "ui/popup_templates.h"
#include <stdint.h>

void popup_init(popup_window_data *popup_window, const window_data *main_window, popup_type window_type);
void popup_window_action(window_data *main_window, popup_window_data *popup_window, popup_type window_tpye,
						 device *action_device);
void popup_clean_up(popup_window_data *popup_window);

#endif
