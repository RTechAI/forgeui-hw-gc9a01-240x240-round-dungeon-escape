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
        lv_obj_t *row = label(list, "", 4 + (int)(i - first) * 30,
                              i == selected ? 0x21D4C2 : 0x75929E);
        lv_label_set_text_fmt(row, "%s%s", i == selected ? "> " : "", entries[i].name);
    }
    lv_label_set_text(hint, entries[selected].start ? "PRESS TO PLAY" : "COMING SOON");
}

void game_selector_show(const arcade_game_t *games, size_t count, size_t selected)
{
    entries = games;
    entry_count = count;
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    label(screen, "FORGEUI", 29, 0x21D4C2);
    label(screen, "MICRO ARCADE", 49, 0xE1F7F8);
    list = lv_obj_create(screen);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 188, 94);
    lv_obj_set_pos(list, 26, 79);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    hint = label(screen, "", 185, 0x21D4C2);
    label(screen, "UP / DOWN", 205, 0x75929E);
    if (count) game_selector_select(selected < count ? selected : 0);
}

void game_selector_unavailable(void)
{
    lv_label_set_text(hint, "COMING SOON");
}
