#pragma once

#include <stdbool.h>
#include "lvgl.h"

bool dungeon_renderer_position_clear(float x, float y);
void dungeon_renderer_update(float x, float y, float angle, uint32_t time_ms);
void dungeon_renderer_draw(lv_event_t *event);

/* Optional 12x12 level tiles: 0 is walkable, 1..3 are wall materials.
 * NULL restores the original renderer experiment. Caller owns callback lifetime. */
void dungeon_renderer_set_tiles(int (*read_tile)(int x, int y));
bool dungeon_renderer_project(float x, float y, float *screen_x, float *depth);
float dungeon_renderer_depth(int screen_x);
