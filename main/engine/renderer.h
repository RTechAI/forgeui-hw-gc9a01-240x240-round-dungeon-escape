#pragma once

#include <stdbool.h>
#include "lvgl.h"

bool microdoom_position_clear(float x, float y);
void microdoom_renderer_update(float x, float y, float angle, uint32_t time_ms);
void microdoom_renderer_draw(lv_event_t *event);
