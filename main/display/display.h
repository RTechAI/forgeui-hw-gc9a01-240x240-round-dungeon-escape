#pragma once

#include "gc9a01.h"
#include "lvgl.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

extern lv_display_t *display;

bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx);

/* Initialize LVGL, invoke the application, then start the handler task. */
void displayConfig(void (*start_ui)(void));
