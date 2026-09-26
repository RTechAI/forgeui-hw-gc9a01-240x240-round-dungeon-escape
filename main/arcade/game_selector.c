#include "game_selector.h"
#include "lvgl.h"

static const arcade_game_t *entries;
static size_t entry_count;
static lv_obj_t *list;
static lv_obj_t *hint;

static lv_obj_t *label(lv_obj_t *parent, const char *text, int y, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y);
    return obj;
}

void game_selector_select(size_t selected)
{
    /* A three-row window keeps an arbitrary registry inside the round bezel. */
    lv_obj_clean(list);
    size_t first = selected > 0 ? selected - 1 : 0;
    if (entry_count > 3 && first > entry_count - 3) first = entry_count - 3;
    for (size_t i = first; i < entry_count && i < first + 3; ++i) {
        lv_obj_t *row = label(list, "", 7 + (int)(i - first) * 31,
                              i == selected ? 0x21D4C2 : 0x75929E);
        lv_label_set_text_fmt(row, "%s%s", i == selected ? "> " : "", entries[i].name);
        if (i == selected) {
            lv_obj_set_width(row, 184);
            lv_obj_set_style_text_align(row, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_pad_ver(row, 4, 0);
            lv_obj_set_style_radius(row, 8, 0);
            lv_obj_set_style_bg_color(row, lv_color_hex(0x12343F), 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        }
    }
    lv_label_set_text(hint, entries[selected].start ? "PRESS TO PLAY" : "COMING SOON");
}

void game_selector_show(const arcade_game_t *games, size_t count, size_t selected)
{
    entries = games;
    entry_count = count;
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *rim = lv_obj_create(screen);
    lv_obj_remove_style_all(rim);
    lv_obj_set_size(rim, 234, 234);
    lv_obj_center(rim);
    lv_obj_set_style_radius(rim, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(rim, 1, 0);
    lv_obj_set_style_border_color(rim, lv_color_hex(0x215460), 0);
    lv_obj_remove_flag(rim, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    label(screen, "FORGEUI", 24, 0x21D4C2);
    label(screen, "MICRO GAMES", 43, 0xE1F7F8);
    label(screen, "DUNGEON / LEVEL 1", 65, 0x75929E);
    list = lv_obj_create(screen);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 196, 103);
    lv_obj_set_pos(list, 22, 84);
    lv_obj_remove_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    hint = label(screen, "", 188, 0x21D4C2);
    label(screen, "UP / DOWN", 205, 0x75929E);
    if (count) game_selector_select(selected < count ? selected : 0);
}

void game_selector_unavailable(void)
{
    lv_label_set_text(hint, "COMING SOON");
}
