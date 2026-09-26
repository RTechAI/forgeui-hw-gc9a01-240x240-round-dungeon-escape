#pragma once

#include "gc9a01.h"
#include "lvgl.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

extern lv_disp_drv_t disp_drv;

bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx);

/* Initialise LVGL and start the ForgeUI MicroSnake game. */
void displayConfig(void);
