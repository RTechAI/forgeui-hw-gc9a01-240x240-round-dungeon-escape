#pragma once

#include "input/micro_input.h"

void dungeon_camera_start(void);
void dungeon_camera_tick(const micro_input_state_t *input);
void dungeon_camera_stop(void);

/* Camera access for a game-owned level and HUD. Movement stays in the engine. */
void dungeon_camera_set_pose(float x, float y, float angle);
void dungeon_camera_get_pose(float *x, float *y, float *angle);
