#include "display.h"
#include "game/microsnake.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "forgeui_display";
lv_disp_drv_t disp_drv;

static volatile uint32_t flush_count;
static volatile uint32_t flush_ready_count;

bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx)
{
    flush_ready_count++;
    lv_disp_flush_ready((lv_disp_drv_t *)user_ctx);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *color_map)
{
    flush_count++;
    if (flush_count <= 3) {
        ESP_LOGI(TAG, "Flush %lu: (%d,%d)-(%d,%d), buffer=%p",
                 (unsigned long)flush_count, area->x1, area->y1, area->x2, area->y2, color_map);
    }

    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)driver->user_data;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
                                               area->x2 + 1, area->y2 + 1, color_map));
}

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    uint32_t iterations = 0;
    ESP_LOGI(TAG, "LVGL handler task started");
    for (;;) {
        lv_timer_handler();
        if (++iterations % 200 == 0) {
            ESP_LOGI(TAG, "LVGL handler alive; flush=%lu ready=%lu",
                     (unsigned long)flush_count, (unsigned long)flush_ready_count);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void displayConfig(void)
{
    static lv_disp_draw_buf_t draw_buffer;
    lv_color_t *buffer_a = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buffer_b = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buffer_a && buffer_b);

    ESP_LOGI(TAG, "Initialize LVGL; buffers=%p,%p", buffer_a, buffer_b);
    lv_init();
    lv_disp_draw_buf_init(&draw_buffer, buffer_a, buffer_b, EXAMPLE_LCD_H_RES * 20);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buffer;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);
    ESP_LOGI(TAG, "LVGL display driver registered: %dx%d", EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
    ESP_LOGI(TAG, "LVGL tick timer started at %d ms", EXAMPLE_LVGL_TICK_PERIOD_MS);

    microsnake_start();
    assert(xTaskCreate(lvgl_task, "lvgl", 4096, NULL, 4, NULL) == pdPASS);
}
