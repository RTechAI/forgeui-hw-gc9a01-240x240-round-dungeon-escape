#pragma once
#include "input/micro_input.h"

void micropong_start(void);
void micropong_tick(const micro_input_state_t *input);
void micropong_stop(void);
