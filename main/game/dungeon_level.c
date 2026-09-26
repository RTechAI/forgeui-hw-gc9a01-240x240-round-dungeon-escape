#include "dungeon_level.h"

/* 1 = wall; 0 = floor; S = start; K = key; D = locked exit.
 * Exit/start room at left, key room at upper right. The lower corridor
 * joins them; its southern branch is a small optional side room.
 * Route: south from S, east along row 6, north into the key room.
 * Return west along row 6, then north through the start room to D. */
static const char level[DUNGEON_SIZE][DUNGEON_SIZE + 1] = {
    "111111111111",
    "111111111111",
    "11D111100001",
    "100011100K01",
    "10S011100001",
    "100011111011",
    "110000000011",
    "111101111111",
    "111100001111",
    "111100001111",
    "111111111111",
    "111111111111",
};

static bool has_key, door_open;

void dungeon_level_reset(void)
{
    has_key = false;
    door_open = false;
}

int dungeon_level_tile(int x, int y)
{
    if (x < 0 || y < 0 || x >= DUNGEON_SIZE || y >= DUNGEON_SIZE) return 1;
    const char cell = level[y][x];
    if (cell == 'D') return door_open ? 0 : 3;
    return cell == '1' ? 2 : 0;
}

bool dungeon_level_has_key(void) { return has_key; }
bool dungeon_level_door_open(void) { return door_open; }

bool dungeon_level_pickup(float x, float y)
{
    const float dx = x - DUNGEON_KEY_X, dy = y - DUNGEON_KEY_Y;
    if (has_key || dx * dx + dy * dy > 0.55f * 0.55f) return false;
    has_key = true;
    return true;
}

bool dungeon_level_near_door(float x, float y)
{
    /* Only from the exit room, never through the neighbouring wall. */
    return x > 2.0f && x < 3.0f && y >= 3.0f && y < 4.15f;
}

void dungeon_level_unlock(void)
{
    if (has_key) door_open = true;
}

bool dungeon_level_escaped(float x, float y)
{
    return has_key && door_open && x > 2.0f && x < 3.0f && y < 3.0f && y > 2.0f;
}
