#pragma once

#include <stdbool.h>

#define DUNGEON_SIZE 12
#define DUNGEON_START_X 2.5f
#define DUNGEON_START_Y 4.5f
#define DUNGEON_KEY_X 9.5f
#define DUNGEON_KEY_Y 3.5f
#define DUNGEON_EXIT_X 2.5f
#define DUNGEON_EXIT_Y 2.5f

void dungeon_level_reset(void);
int dungeon_level_tile(int x, int y);
bool dungeon_level_has_key(void);
bool dungeon_level_door_open(void);
bool dungeon_level_pickup(float x, float y);
bool dungeon_level_near_door(float x, float y);
void dungeon_level_unlock(void);
bool dungeon_level_escaped(float x, float y);
