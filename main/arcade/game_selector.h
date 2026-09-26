#pragma once
#include "arcade.h"

void game_selector_show(const arcade_game_t *games, size_t count, size_t selected);
void game_selector_select(size_t selected);
void game_selector_unavailable(void);
