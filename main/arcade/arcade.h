#pragma once
#include <stddef.h>
#include "input/micro_input.h"

/* All callbacks run on the LVGL task. A game owns its screen children until
 * stop; stop must cancel any resources it owns before the screen is cleaned.
 * tick receives the single shared input sample; it must not poll hardware.
 * A NULL start callback marks a placeholder. */
typedef struct {
    const char *name;
    void (*start)(void);
    void (*tick)(const micro_input_state_t *input);
    void (*stop)(void);
} arcade_game_t;

void arcade_start(void);
