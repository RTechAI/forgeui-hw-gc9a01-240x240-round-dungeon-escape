#include "dungeon_camera.h"
#include "dungeon_renderer.h"

#include <math.h>

static lv_obj_t *view;
static float player_x, player_y, angle;
static uint32_t previous_tick, previous_frame;

static float axis(int raw)
{
    /* Match the inherited input thresholds, with proportional speed outside. */
    if (raw < 1200) return fmaxf(-1.0f, (raw - 1200) / 1200.0f);
    if (raw > 2900) return fminf(1.0f, (raw - 2900) / 1195.0f);
    return 0.0f;
}

void dungeon_camera_set_pose(float x, float y, float direction)
{
    player_x = x;
    player_y = y;
    angle = direction;
    previous_tick = previous_frame = lv_tick_get();
    dungeon_renderer_update(player_x, player_y, angle, previous_tick);
    if (view) lv_obj_invalidate(view);
}

void dungeon_camera_get_pose(float *x, float *y, float *direction)
{
    *x = player_x;
    *y = player_y;
    *direction = angle;
}

void dungeon_camera_start(void)
{
    player_x = 2.5f;
    player_y = 3.5f;
    angle = 0.0f;
    previous_tick = previous_frame = lv_tick_get();
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    view = lv_obj_create(screen);
    lv_obj_remove_style_all(view);
    lv_obj_set_size(view, 240, 240);
    lv_obj_set_pos(view, 0, 0);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(view, dungeon_renderer_draw, LV_EVENT_DRAW_MAIN, NULL);
    dungeon_renderer_update(player_x, player_y, angle, previous_tick);
    /* The game layer owns titles, objectives and HUD. */
}

void dungeon_camera_tick(const micro_input_state_t *input)
{
    if (!view) return;
    const uint32_t now = lv_tick_get();
    uint32_t elapsed = now - previous_tick;
    previous_tick = now;
    if (elapsed > 50) elapsed = 50; /* Bound movement after slow display flushes. */
    const float dt = elapsed * 0.001f;
    if (input->axes_valid) {
        angle += axis(input->x_raw) * 1.8f * dt;
        if (angle > 3.14159265f) angle -= 6.2831853f;
        if (angle < -3.14159265f) angle += 6.2831853f;
        const float step = -axis(input->y_raw) * 1.6f * dt;
        const float next_x = player_x + cosf(angle) * step;
        const float next_y = player_y + sinf(angle) * step;
        /* Separate axes allow sliding along walls; radius prevents clipping. */
        if (dungeon_renderer_position_clear(next_x, player_y)) player_x = next_x;
        if (dungeon_renderer_position_clear(player_x, next_y)) player_y = next_y;
    }
    if (now - previous_frame < 50) return; /* Request at most 20 scene frames/s. */
    previous_frame = now;
    dungeon_renderer_update(player_x, player_y, angle, now);
    lv_obj_invalidate(view);
}

void dungeon_camera_stop(void)
{
    /* Host deletes all screen children. No private tasks, timers or buffers. */
    view = NULL;
}
