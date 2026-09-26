/* ForgeUI Dungeon Escape: one local level, no assets or extra tasks. */
#include "dungeon_escape.h"
#include "dungeon_level.h"
#include "engine/dungeon_camera.h"
#include "engine/dungeon_renderer.h"
#include "esp_log.h"
#include "lvgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEAL 0x21D4C2
#define GOLD 0xFFE080
#define INK 0x07111F
#define PI 3.14159265f

typedef enum { INTRO, EXPLORING, UNLOCKING, ESCAPING, COMPLETE } phase_t;
static phase_t phase;
static lv_obj_t *overlay, *card, *title, *subtitle, *instructions;
static lv_obj_t *hud, *key_label, *message, *guide;
static uint32_t phase_started, last_frame, effect_started, message_started;
static uint32_t button_started;
static bool effect_active, tracking_press, button_armed;
static uint32_t effect_color;
static float player_x, player_y, player_angle;
static char guide_text[40];

static void visible(lv_obj_t *obj, bool show)
{
    if (show) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *container(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 240, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *text(lv_obj_t *parent, const char *value, int y, int width, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, value);
    lv_obj_set_width(obj, width);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(INK), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_set_style_pad_ver(obj, 2, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y);
    return obj;
}

/* LVGL clips each primitive to its DMA strip; reject invisible rectangles
 * before allocating draw tasks. Coordinates are display-local (240x240). */
static void rect(lv_layer_t *layer, int x1, int y1, int x2, int y2,
                 uint32_t color, lv_opa_t opacity)
{
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > 239) x2 = 239;
    if (y2 > 239) y2 = 239;
    if (x1 > x2 || y1 > y2 || !opacity) return;
    if (x2 < layer->_clip_area.x1 || x1 > layer->_clip_area.x2 ||
        y2 < layer->_clip_area.y1 || y1 > layer->_clip_area.y2) return;
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_hex(color);
    d.bg_opa = opacity;
    const lv_area_t area = {x1, y1, x2, y2};
    lv_draw_rect(layer, &d, &area);
}

/* Tiny primitive glyphs projected into the world. Wall-depth clipping avoids
 * seeing the key/exit through walls; no textures or image decoder required. */
static const char key_glyph[9][8] = {
    "0111000", "1101100", "1101100", "0111000", "0010000",
    "0010000", "0011100", "0010000", "0011100"
};

static char door_pixel(int x, int y, float opening)
{
    if (x == 0 || x == 8 || y == 0 || y == 13) return '2';
    /* The leaf slides upward; the frame remains as the escape threshold. */
    if (y >= 13 - (int)(12 * opening)) return '0';
    if (opening == 0 && x == 4 && (y == 6 || y == 7)) return '3';
    return '1';
}

static void marker(lv_layer_t *layer, bool key, uint32_t now)
{
    float sx, depth;
    /* Door sits just in front of its blocking tile, facing the exit room. */
    const float wx = key ? DUNGEON_KEY_X : DUNGEON_EXIT_X;
    const float wy = key ? DUNGEON_KEY_Y : 3.025f;
    if (!dungeon_renderer_project(wx, wy, &sx, &depth)) return;
    const int cols = key ? 7 : 9, rows = key ? 9 : 14;
    const float width = (key ? 0.38f : 0.82f) * 181.81818f / depth;
    const float height = (key ? 0.46f : 1.0f) * 180.0f / depth;
    const float bottom = 120.0f + (key ? 20.0f : 90.0f) / depth;
    const float left = sx - width * 0.5f, top = bottom - height;
    if (left > 239 || left + width < 0) return;
    float opening = dungeon_level_door_open() ? 1.0f : 0.0f;
    if (phase == UNLOCKING) opening = fminf(1.0f, (now - phase_started) / 650.0f);
    /* Group visible screen columns into spans, keeping draw-task count small
     * even when the door fills the screen. Wall clipping uses cached rays. */
    for (int gx = 0; gx < cols; ++gx) {
        int first = (int)ceilf(left + width * gx / cols - 0.5f);
        int last = (int)ceilf(left + width * (gx + 1) / cols - 0.5f) - 1;
        if (first < 0) first = 0;
        if (last > 239) last = 239;
        int run = -1;
        for (int px = first; px <= last + 1; ++px) {
            const bool shown = px <= last && depth < dungeon_renderer_depth(px);
            if (shown && run < 0) run = px;
            if (shown || run < 0) continue;
            for (int gy = 0; gy < rows; ++gy) {
                const char pixel = key ? key_glyph[gy][gx] : door_pixel(gx, gy, opening);
                if (pixel == '0') continue;
                uint32_t color = key ? GOLD : pixel == '2' ? TEAL : pixel == '3' ? GOLD : 0x173C43;
                const int y1 = (int)floorf(top + height * gy / rows);
                const int y2 = (int)floorf(top + height * (gy + 1) / rows) - 1;
                rect(layer, run, y1, px - 1, y2, color, LV_OPA_COVER);
            }
            run = -1;
        }
    }
}

static void sparks(lv_layer_t *layer, uint32_t elapsed, uint32_t color)
{
    if (elapsed >= 950) return;
    const float t = elapsed / 950.0f;
    for (int i = 0; i < 12; ++i) {
        const float angle = i * PI / 6;
        const float radius = 10 + t * (55 + (i % 3) * 14);
        const int x = 120 + (int)(cosf(angle) * radius);
        const int y = 118 + (int)(sinf(angle) * radius + 15 * t * t);
        rect(layer, x, y, x + 2, y + 2, color, (lv_opa_t)(255 * (1.0f - t)));
    }
}

static void paint(lv_event_t *event)
{
    lv_layer_t *layer = lv_event_get_layer(event);
    const uint32_t now = lv_tick_get();
    if (phase == INTRO || phase == COMPLETE) {
        rect(layer, 0, 0, 239, 239, INK, LV_OPA_COVER);
        rect(layer, 49, 59, 190, 60, TEAL, LV_OPA_COVER);
        rect(layer, 49, 176, 190, 177, TEAL, LV_OPA_COVER);
        if (phase == COMPLETE) sparks(layer, now - phase_started, TEAL);
        return;
    }
    /* Far-to-near ordering for the two world markers. */
    float sx, key_depth = 0, exit_depth = 0;
    dungeon_renderer_project(DUNGEON_KEY_X, DUNGEON_KEY_Y, &sx, &key_depth);
    dungeon_renderer_project(DUNGEON_EXIT_X, 3.025f, &sx, &exit_depth);
    if (key_depth > exit_depth) {
        if (!dungeon_level_has_key()) marker(layer, true, now);
        marker(layer, false, now);
    } else {
        marker(layer, false, now);
        if (!dungeon_level_has_key()) marker(layer, true, now);
    }
    if (effect_active) {
        const uint32_t elapsed = now - effect_started;
        if (elapsed < 220) rect(layer, 0, 0, 239, 239, effect_color,
                               (lv_opa_t)(85 * (220 - elapsed) / 220));
        sparks(layer, elapsed, effect_color);
    }
    if (phase == ESCAPING) {
        const uint32_t elapsed = now - phase_started;
        rect(layer, 0, 0, 239, 239, TEAL, (lv_opa_t)(elapsed * 220 / 900));
        sparks(layer, elapsed, 0xE8FFFA);
    }
}

static void notify(const char *value, uint32_t color, uint32_t now)
{
    lv_label_set_text(message, value);
    lv_obj_set_style_text_color(message, lv_color_hex(color), 0);
    message_started = now;
}

static void celebrate(uint32_t color, uint32_t now)
{
    effect_active = true;
    effect_started = now;
    effect_color = color;
}

static void introduction(uint32_t now)
{
    dungeon_level_reset();
    dungeon_camera_set_pose(DUNGEON_START_X, DUNGEON_START_Y, PI * 0.5f);
    phase = INTRO;
    phase_started = last_frame = now;
    effect_active = tracking_press = button_armed = false;
    guide_text[0] = '\0';
    lv_label_set_text(key_label, "KEY: NOT FOUND");
    lv_obj_set_style_text_color(key_label, lv_color_hex(GOLD), 0);
    lv_label_set_text(title, "DUNGEON\nESCAPE");
    lv_label_set_text(subtitle, "LEVEL 1");
    lv_label_set_text(instructions, "Find the key\nand escape.");
    visible(card, true);
    visible(hud, false);
    lv_obj_invalidate(overlay);
    ESP_LOGI("dungeon", "Level 1 ready; key=(9.5,3.5), exit=(2.5,2.5)");
}

void dungeon_escape_start(void)
{
    dungeon_level_reset();
    dungeon_renderer_set_tiles(dungeon_level_tile);
    dungeon_camera_start();
    overlay = container(lv_screen_active());
    lv_obj_add_event_cb(overlay, paint, LV_EVENT_DRAW_MAIN, NULL);
    hud = container(overlay);
    text(hud, "LEVEL: 1", 17, 118, TEAL);
    key_label = text(hud, "KEY: NOT FOUND", 37, 168, GOLD);
    guide = text(hud, "", 166, 182, TEAL);
    message = text(hud, "", 185, 170, GOLD);
    text(hud, "HOLD: MENU", 206, 128, 0x91ADB6);
    card = container(overlay);
    text(card, "FORGEUI", 31, 122, TEAL);
    title = text(card, "", 76, 172, 0xE8FFFA);
    subtitle = text(card, "", 119, 176, TEAL);
    instructions = text(card, "", 140, 176, 0xAFC9D0);
    text(card, "HOLD: MENU", 194, 140, 0x91ADB6);
    introduction(lv_tick_get());
}

static void update_guide(void)
{
    const float tx = dungeon_level_has_key() ? DUNGEON_EXIT_X : DUNGEON_KEY_X;
    const float ty = dungeon_level_has_key() ? DUNGEON_EXIT_Y : DUNGEON_KEY_Y;
    float relative = atan2f(ty - player_y, tx - player_x) - player_angle;
    while (relative > PI) relative -= 2 * PI;
    while (relative < -PI) relative += 2 * PI;
    const char *direction = fabsf(relative) > 2.5f ? "BEHIND" :
                            relative > 0.35f ? "RIGHT >" :
                            relative < -0.35f ? "< LEFT" : "AHEAD";
    char next[40];
    snprintf(next, sizeof(next), "%s: %s", dungeon_level_has_key() ? "EXIT" : "KEY", direction);
    if (strcmp(next, guide_text)) {
        strcpy(guide_text, next);
        lv_label_set_text(guide, guide_text);
    }
}

void dungeon_escape_tick(const micro_input_state_t *input)
{
    if (!overlay) return;
    const uint32_t now = lv_tick_get();
    /* A short release replays; a long hold belongs exclusively to the host.
     * The launch press must be released before any game button action. */
    bool tap = false;
    if (!input->button_pressed) {
        tap = tracking_press && now - button_started >= 30 && now - button_started < 600;
        tracking_press = false;
        button_armed = true;
    } else if (button_armed && !tracking_press) {
        tracking_press = true;
        button_started = now;
    }
    micro_input_state_t camera_input = *input;
    if (phase != EXPLORING) camera_input.axes_valid = false;
    dungeon_camera_tick(&camera_input);
    dungeon_camera_get_pose(&player_x, &player_y, &player_angle);

    if (phase == INTRO && now - phase_started >= 3000) {
        phase = EXPLORING;
        phase_started = now;
        visible(card, false);
        visible(hud, true);
        notify("FIND THE KEY", GOLD, now);
        ESP_LOGI("dungeon", "Exploring");
    } else if (phase == EXPLORING) {
        if (dungeon_level_pickup(player_x, player_y)) {
            lv_label_set_text(key_label, "KEY: FOUND");
            lv_obj_set_style_text_color(key_label, lv_color_hex(TEAL), 0);
            notify("KEY FOUND!", GOLD, now);
            celebrate(GOLD, now);
            ESP_LOGI("dungeon", "Key collected");
        }
        if (dungeon_level_near_door(player_x, player_y)) {
            if (!dungeon_level_has_key()) {
                if (now - message_started >= 2500) notify("LOCKED - FIND KEY", GOLD, now);
            } else if (!dungeon_level_door_open()) {
                phase = UNLOCKING;
                phase_started = now;
                notify("UNLOCKING...", TEAL, now);
                ESP_LOGI("dungeon", "Unlocking exit");
            }
        }
        if (dungeon_level_escaped(player_x, player_y)) {
            phase = ESCAPING;
            phase_started = now;
            visible(hud, false);
            ESP_LOGI("dungeon", "Escape sequence");
        }
    } else if (phase == UNLOCKING && now - phase_started >= 650) {
        dungeon_level_unlock();
        phase = EXPLORING;
        phase_started = now;
        notify("EXIT OPEN - ENTER", TEAL, now);
        celebrate(TEAL, now);
        /* Refresh wall depth immediately when the door ceases blocking. */
        dungeon_camera_set_pose(player_x, player_y, player_angle);
        ESP_LOGI("dungeon", "Exit unlocked");
    } else if (phase == ESCAPING && now - phase_started >= 900) {
        phase = COMPLETE;
        phase_started = now;
        lv_label_set_text(title, "ESCAPED!");
        lv_label_set_text(subtitle, "LEVEL COMPLETE");
        lv_label_set_text(instructions, "PRESS TO REPLAY");
        visible(card, true);
        tracking_press = button_armed = false;
        ESP_LOGI("dungeon", "LEVEL COMPLETE");
    } else if (phase == COMPLETE && tap && now - phase_started >= 500) {
        introduction(now);
    }
    if (now - last_frame < 50) return;
    last_frame = now;
    if (effect_active && now - effect_started >= 950) effect_active = false;
    if (phase == EXPLORING) {
        update_guide();
        if (now - message_started >= 2500) {
            const float dx = player_x - DUNGEON_KEY_X, dy = player_y - DUNGEON_KEY_Y;
            const char *hint = !dungeon_level_has_key() && dx * dx + dy * dy < 2.0f ?
                               "WALK INTO KEY" : dungeon_level_has_key() ? "RETURN TO EXIT" : "EXPLORE THE HALLS";
            if (strcmp(lv_label_get_text(message), hint)) lv_label_set_text(message, hint);
        }
    }
    /* No object creation, animation handles or additional timers per frame. */
    lv_obj_invalidate(overlay);
}

void dungeon_escape_stop(void)
{
    dungeon_camera_stop();
    dungeon_renderer_set_tiles(NULL);
    overlay = NULL;
    /* The arcade host deletes all children after stop. No callbacks survive. */
}
