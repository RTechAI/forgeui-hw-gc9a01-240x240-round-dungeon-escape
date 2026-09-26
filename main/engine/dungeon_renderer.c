/* ForgeUI Dungeon Escape / ForgeUI Micro Games. Procedural raycasting proof, no external assets. */
#include "dungeon_renderer.h"

#include <math.h>

#define VIEW_SIZE 240
#define RAY_COUNT 120
#define MAP_SIZE 12

/* Closed boundary, interconnected corridors and a central room. */
static const char map[MAP_SIZE][MAP_SIZE + 1] = {
    "111111111111",
    "100000000001",
    "102220033301",
    "100000000001",
    "101100110101",
    "100000000001",
    "100000000001",
    "103300220101",
    "100000000001",
    "101110011101",
    "100000000001",
    "111111111111",
};

typedef struct {
    int16_t top, bottom;
    lv_color_t wall, trim;
} wall_column_t;

/* Cached geometry is reused across LVGL's DMA-sized redraw strips. */
static wall_column_t columns[RAY_COUNT];
static float depths[RAY_COUNT];
static float camera_x, camera_y, camera_angle;
static int (*level_tile)(int x, int y);

void dungeon_renderer_set_tiles(int (*read_tile)(int x, int y))
{
    level_tile = read_tile;
}

bool dungeon_renderer_project(float x, float y, float *screen_x, float *depth)
{
    const float dx = x - camera_x, dy = y - camera_y;
    const float c = cosf(camera_angle), s = sinf(camera_angle);
    *depth = dx * c + dy * s;
    if (*depth <= 0.12f) return false;
    *screen_x = 120.0f + 120.0f * (-dx * s + dy * c) / (0.66f * *depth);
    return true;
}

float dungeon_renderer_depth(int screen_x)
{
    if (screen_x < 0 || screen_x >= VIEW_SIZE) return 0;
    return depths[screen_x / 2];
}

static int tile(int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE) return 1;
    if (level_tile) return level_tile(x, y);
    return map[y][x] - '0';
}

bool dungeon_renderer_position_clear(float x, float y)
{
    const float radius = 0.20f;
    return !tile((int)floorf(x - radius), (int)floorf(y - radius)) &&
           !tile((int)floorf(x + radius), (int)floorf(y - radius)) &&
           !tile((int)floorf(x - radius), (int)floorf(y + radius)) &&
           !tile((int)floorf(x + radius), (int)floorf(y + radius));
}

static lv_color_t shaded(int material, float light)
{
    static const uint8_t palette[3][3] = {
        {151, 95, 63}, {91, 111, 99}, {158, 53, 36}
    };
    const uint8_t *rgb = palette[(material - 1) % 3];
    return lv_color_make((uint8_t)(rgb[0] * light),
                         (uint8_t)(rgb[1] * light),
                         (uint8_t)(rgb[2] * light));
}

void dungeon_renderer_update(float x, float y, float angle, uint32_t time_ms)
{
    camera_x = x;
    camera_y = y;
    camera_angle = angle;
    const float dx = cosf(angle), dy = sinf(angle);
    const float pulse = 0.90f + 0.10f * sinf((time_ms % 4000) * 0.0015707963f);
    for (int i = 0; i < RAY_COUNT; ++i) {
        const float camera = 2.0f * (i + 0.5f) / RAY_COUNT - 1.0f;
        /* Perpendicular camera plane: ~67 degree horizontal field of view. */
        const float rx = dx - dy * 0.66f * camera;
        const float ry = dy + dx * 0.66f * camera;
        const float delta_x = fabsf(rx) < 0.00001f ? 1.0e6f : fabsf(1.0f / rx);
        const float delta_y = fabsf(ry) < 0.00001f ? 1.0e6f : fabsf(1.0f / ry);
        int mx = (int)x, my = (int)y;
        const int sx = rx < 0 ? -1 : 1, sy = ry < 0 ? -1 : 1;
        float side_x = (rx < 0 ? x - mx : mx + 1.0f - x) * delta_x;
        float side_y = (ry < 0 ? y - my : my + 1.0f - y) * delta_y;
        float distance = 1.0f;
        int side = 0, material = 1;
        /* At most 24 boundary crossings in this map; extra bound is defensive. */
        for (int step = 0; step < 2 * MAP_SIZE + 2; ++step) {
            if (side_x < side_y) {
                distance = side_x;
                side_x += delta_x;
                mx += sx;
                side = 0;
            } else {
                distance = side_y;
                side_y += delta_y;
                my += sy;
                side = 1;
            }
            material = tile(mx, my);
            if (material) break;
        }
        if (!material) material = 1;
        if (distance < 0.15f) distance = 0.15f;
        depths[i] = distance;
        /* Ray parameter is perpendicular depth: avoids fish-eye distortion. */
        const int height = (int)(180.0f / distance);
        wall_column_t *column = &columns[i];
        column->top = 120 - height / 2;
        column->bottom = 120 + height / 2;
        float u = side ? x + distance * rx : y + distance * ry;
        u -= floorf(u);
        float light = pulse / (1.0f + distance * 0.16f);
        if (side) light *= 0.72f;
        /* Procedural vertical panel joints reveal perspective during movement. */
        if (u < 0.045f || u > 0.955f) light *= 0.48f;
        column->wall = shaded(material, light);
        column->trim = shaded(material, light * 0.45f);
    }
}

static void rectangle(lv_layer_t *layer, lv_draw_rect_dsc_t *style,
                      const lv_area_t *origin, int x1, int y1, int x2, int y2,
                      lv_color_t color)
{
    if (y1 < 0) y1 = 0;
    if (y2 >= VIEW_SIZE) y2 = VIEW_SIZE - 1;
    if (y1 > y2) return;
    lv_area_t area = {origin->x1 + x1, origin->y1 + y1,
                      origin->x1 + x2, origin->y1 + y2};
    /* Cull against the current strip before creating LVGL 9 draw tasks. */
    if (area.x2 < layer->_clip_area.x1 || area.x1 > layer->_clip_area.x2 ||
        area.y2 < layer->_clip_area.y1 || area.y1 > layer->_clip_area.y2) return;
    style->bg_color = color;
    lv_draw_rect(layer, style, &area);
}

void dungeon_renderer_draw(lv_event_t *event)
{
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t origin;
    lv_obj_get_coords(lv_event_get_target(event), &origin);
    lv_draw_rect_dsc_t style;
    lv_draw_rect_dsc_init(&style);
    style.bg_opa = LV_OPA_COVER;
    /* Full 240x240 surface: the physical circular bezel supplies the mask. */
    rectangle(layer, &style, &origin, 0, 0, 239, 119, lv_color_hex(0x11151C));
    for (int band = 0; band < 12; ++band) {
        const int shade = 22 + band * 3;
        rectangle(layer, &style, &origin, 0, 120 + band * 10, 239, 129 + band * 10,
                  lv_color_make(shade, shade - 4, shade - 8));
    }
    for (int i = 0; i < RAY_COUNT; ++i) {
        const wall_column_t *column = &columns[i];
        const int height = column->bottom - column->top + 1;
        const int trim = height / 16 + 1;
        rectangle(layer, &style, &origin, i * 2, column->top, i * 2 + 1,
                  column->bottom, column->wall);
        rectangle(layer, &style, &origin, i * 2, column->top, i * 2 + 1,
                  column->top + trim, column->trim);
        rectangle(layer, &style, &origin, i * 2, column->bottom - trim, i * 2 + 1,
                  column->bottom, column->trim);
        /* A waist-height wall seam follows the same perspective projection. */
        rectangle(layer, &style, &origin, i * 2, 120 + height / 8, i * 2 + 1,
                  120 + height / 8 + trim / 3, column->trim);
    }
}
