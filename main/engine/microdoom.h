#pragma once

#include "input/micro_input.h"

void microdoom_start(void);
void microdoom_tick(const micro_input_state_t *input);
void microdoom_stop(void);
