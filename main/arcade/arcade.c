#include "arcade.h"
#include "game_selector.h"
#include "boot.h"
#include "games/microsnake/microsnake.h"
#include "games/microasteroids/microasteroids.h"
#include "lvgl.h"

static const arcade_game_t games[] = {
    {"MicroAsteroids", microasteroids_start, microasteroids_tick, microasteroids_stop},
    {"MicroSnake", microsnake_start, microsnake_tick, microsnake_stop},
    {"Coming Soon", NULL, NULL, NULL},
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
        if (lv_tick_elaps(boot_started) >= 2400) show_selector();
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

void arcade_start(void)
{
    state = BOOT;
    boot_started = lv_tick_get();
    arcade_boot_show();
    lv_timer_create(arcade_tick, 10, NULL);
}
