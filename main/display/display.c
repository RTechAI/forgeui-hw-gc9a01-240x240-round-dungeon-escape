#include "display.h"


#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "forgeui_display";
lv_display_t *display;

static volatile uint32_t flush_count;
static volatile uint32_t flush_ready_count;

bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx)
{
    flush_ready_count++;
    /* Panel IO is initialized first; display is set before any LVGL flush. */
    lv_display_flush_ready(*(lv_display_t **)user_ctx);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    flush_count++;
    if (flush_count <= 3) {
        ESP_LOGI(TAG, "Flush %lu: (%d,%d)-(%d,%d), buffer=%p",
                 (unsigned long)flush_count, area->x1, area->y1, area->x2, area->y2, color_map);
    }

    esp_lcd_panel_handle_t panel = lv_display_get_user_data(disp);
    /* LVGL 9 replaces LV_COLOR_16_SWAP with explicit RGB565 transport swapping. */
    lv_draw_sw_rgb565_swap(color_map, lv_area_get_width(area) * lv_area_get_height(area));
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
        /* At 100 Hz, 5 ms rounds to zero: always let the idle task run. */
        vTaskDelay(pdMS_TO_TICKS(5) > 0 ? pdMS_TO_TICKS(5) : 1);
    }
}

void displayConfig(void (*start_ui)(void))
{
    const size_t buffer_size = EXAMPLE_LCD_H_RES * 20 * sizeof(uint16_t);
    uint8_t *buffer_a = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);
    uint8_t *buffer_b = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);
    assert(buffer_a && buffer_b);

    ESP_LOGI(TAG, "Initialize LVGL; buffers=%p,%p", buffer_a, buffer_b);
    lv_init();
    display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, buffer_a, buffer_b, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panel_handle);
    lv_display_set_flush_cb(display, lvgl_flush_cb);
    ESP_LOGI(TAG, "LVGL display driver registered: %dx%d", EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
    ESP_LOGI(TAG, "LVGL tick timer started at %d ms", EXAMPLE_LVGL_TICK_PERIOD_MS);

    if (start_ui) start_ui();
    assert(xTaskCreate(lvgl_task, "lvgl", 8192, NULL, 4, NULL) == pdPASS);
}
