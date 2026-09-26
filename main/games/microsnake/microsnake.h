#pragma once
#include "input/micro_input.h"

void microsnake_start(void);
void microsnake_tick(const micro_input_state_t *input);
void microsnake_stop(void);
