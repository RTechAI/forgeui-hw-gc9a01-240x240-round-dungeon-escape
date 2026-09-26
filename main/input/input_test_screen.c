#include "input_test_screen.h"
#include "micro_input.h"
#include "lvgl.h"

static lv_obj_t *x_label;
static lv_obj_t *y_label;
static lv_obj_t *button_label;

static lv_obj_t *make_label(const char *text, int y, uint32_t color)
{
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_obj_set_width(label, 196);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}

static void poll_input(lv_timer_t *timer)
{
    (void)timer;
    static uint32_t last_refresh;
    micro_input_state_t state;
    const esp_err_t err = micro_input_read(&state);
    if (lv_tick_elaps(last_refresh) < 50) return;
    last_refresh = lv_tick_get();
    if (err == ESP_OK && state.axes_valid) {
        lv_label_set_text_fmt(x_label, "Joystick X: %4d", state.x_raw);
        lv_label_set_text_fmt(y_label, "Joystick Y: %4d", state.y_raw);
    } else {
        lv_label_set_text(x_label, "Joystick X: ERROR");
        lv_label_set_text(y_label, "Joystick Y: ERROR");
    }
    lv_label_set_text(button_label,
                      state.button_pressed ? "Button: PRESSED" : "Button: RELEASED");
}

void input_test_screen_create(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    make_label("FORGEUI", 30, 0xF4FAFF);
    make_label("MICROSNAKE", 50, 0xF4FAFF);
    make_label("INPUT TEST", 77, 0x21D4C2);
    x_label = make_label("Joystick X: ---", 108, 0xF4FAFF);
    y_label = make_label("Joystick Y: ---", 133, 0xF4FAFF);
    button_label = make_label("Button: ---", 163, 0x21D4C2);
    make_label("RTechAI", 194, 0x75A7C7);
    /* Polling and all label writes run in the existing LVGL handler task. */
    lv_timer_t *timer = lv_timer_create(poll_input, 10, NULL);
    LV_ASSERT_MALLOC(timer);
    poll_input(timer);
}
