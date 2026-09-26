#include "arcade.h"
#include "game_selector.h"
#include "games/microsnake/microsnake.h"
#include "games/micropong/micropong.h"
#include "lvgl.h"

static const arcade_game_t games[] = {
    {"MicroPong", micropong_start, micropong_tick, micropong_stop},
    {"MicroSnake", microsnake_start, microsnake_tick, microsnake_stop},
    {"MicroAsteroids", NULL, NULL, NULL},
};
#define GAME_COUNT (sizeof(games) / sizeof(games[0]))

typedef enum { BOOT, SELECTOR, PLAYING } arcade_state_t;
static arcade_state_t state;
static size_t selected;
static const arcade_game_t *active;
static uint32_t boot_started, button_started;
static bool previous_button, button_armed, hold_tracking;
static int previous_vertical;

static void show_selector(void)
{
    if (active && active->stop) active->stop();
    active = NULL;
    lv_obj_clean(lv_scr_act());
    game_selector_show(games, GAME_COUNT, selected);
    state = SELECTOR;
    button_armed = false;
    hold_tracking = false;
    previous_vertical = 0;
}

static void arcade_tick(lv_timer_t *timer)
{
    (void)timer;
    micro_input_state_t input;
    micro_input_read(&input);
    const bool pressed = input.button_pressed;
    const bool clicked = pressed && !previous_button && button_armed;
    if (!pressed) button_armed = true;
    previous_button = pressed;

    if (state == BOOT) {
        if (lv_tick_elaps(boot_started) >= 1600) show_selector();
        return;
    }
    if (state == PLAYING) {
        if (clicked) {
            button_started = lv_tick_get();
            hold_tracking = true;
        }
        if (!pressed) hold_tracking = false;
        if (hold_tracking && lv_tick_elaps(button_started) >= 1000) {
            show_selector();
            return;
        }
        if (active->tick) active->tick(&input);
        return;
    }

    const int vertical = !input.axes_valid ? 0 :
                         input.y_raw <= 1200 ? -1 : input.y_raw >= 2900 ? 1 : 0;
    if (vertical && vertical != previous_vertical) {
        selected = (selected + GAME_COUNT + vertical) % GAME_COUNT;
        game_selector_select(selected);
    }
    previous_vertical = vertical;
    if (!clicked) return;
    if (!games[selected].start) {
        game_selector_unavailable();
        return;
    }
    active = &games[selected];
    lv_obj_clean(lv_scr_act());
    state = PLAYING;
    button_armed = false; /* Launch press cannot restart or exit the game. */
    hold_tracking = false;
    active->start();
}

static void boot_opacity(void *obj, int32_t value)
{
    lv_obj_set_style_opa(obj, (lv_opa_t)value, 0);
}

static void boot_label(lv_obj_t *screen, const char *text, int y, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(screen);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_letter_space(obj, 2, 0);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, y);
}

void arcade_start(void)
{
    state = BOOT;
    boot_started = lv_tick_get();
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *ring = lv_obj_create(screen);
    lv_obj_set_size(ring, 204, 204);
    lv_obj_center(ring);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(0x21D4C2), 0);
    boot_label(screen, "FORGEUI", -30, 0xE1F7F8);
    boot_label(screen, "MICRO ARCADE", 0, 0x21D4C2);
    boot_label(screen, "READY", 42, 0x75929E);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, ring);
    lv_anim_set_exec_cb(&anim, boot_opacity);
    lv_anim_set_values(&anim, LV_OPA_20, LV_OPA_COVER);
    lv_anim_set_time(&anim, 1000);
    lv_anim_start(&anim);
    lv_timer_create(arcade_tick, 10, NULL);
}
