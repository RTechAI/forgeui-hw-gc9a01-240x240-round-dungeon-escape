/* Lightweight console ident: LVGL arcs, labels and a few orbiting stars. */
#include "boot.h"
#include "lvgl.h"
#include <math.h>

static void sweep(void *obj, int32_t angle)
{
    lv_arc_set_rotation(obj, angle % 360);
    lv_color_t color = lv_color_mix(lv_color_hex(0x45CCFF), lv_color_hex(0x21D4C2),
                                   (angle % 180) * 255 / 180);
    lv_obj_set_style_arc_color(obj, color, LV_PART_MAIN);
}

static void orbit(void *obj, int32_t angle)
{
    float a = angle * 0.01745329252f;
    lv_obj_set_pos(obj, 119 + (int)(cosf(a) * 88), 119 + (int)(sinf(a) * 88));
    lv_obj_set_style_opa(obj, 80 + angle % 170, 0);
}

static void animate(lv_obj_t *obj, lv_anim_exec_xcb_t callback, int from, int to, int duration)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_exec_cb(&anim, callback);
    lv_anim_set_values(&anim, from, to);
    lv_anim_set_time(&anim, duration);
    lv_anim_start(&anim);
    /* LVGL automatically removes object animations when the host cleans up. */
}

static void label(const char *text, int y, uint32_t color, int spacing)
{
    lv_obj_t *obj = lv_label_create(lv_scr_act());
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_letter_space(obj, spacing, 0);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, y);
}

void arcade_boot_show(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 3; ++i) {
        lv_obj_t *ring = lv_arc_create(screen);
        lv_obj_remove_style_all(ring);
        lv_obj_set_size(ring, 234 - i * 19, 234 - i * 19);
        lv_obj_center(ring);
        lv_obj_set_style_arc_width(ring, i == 1 ? 3 : 1, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(ring, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(ring, true, LV_PART_MAIN);
        lv_arc_set_bg_angles(ring, 0, i == 1 ? 110 : 265);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        animate(ring, sweep, i * 120, i * 120 + (i == 1 ? 600 : 300), 2400);
    }
    for (int i = 0; i < 10; ++i) {
        lv_obj_t *star = lv_obj_create(screen);
        lv_obj_remove_style_all(star);
        lv_obj_set_size(star, i % 3 == 0 ? 3 : 2, i % 3 == 0 ? 3 : 2);
        lv_obj_set_style_radius(star, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(star, lv_color_hex(0x80C9D8), 0);
        lv_obj_set_style_bg_opa(star, LV_OPA_COVER, 0);
        animate(star, orbit, i * 36, i * 36 + 18, 2400);
    }
    label("R T E C H A I", -54, 0x75929E, 0);
    label("FORGEUI", -18, 0xE1F7F8, 5);
    label("MICRO ARCADE", 10, 0x21D4C2, 1);
    label("ENTER THE ORBIT", 49, 0x75929E, 0);
}
