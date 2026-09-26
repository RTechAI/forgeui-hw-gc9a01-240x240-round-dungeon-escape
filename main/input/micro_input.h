#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* ESP32-S3 DevKitC-1: power the joystick from 3.3 V. */
#define MICRO_INPUT_X_GPIO 4
#define MICRO_INPUT_Y_GPIO 5
#define MICRO_INPUT_BUTTON_GPIO 6

typedef struct {
    int x_raw; /* Uncalibrated 12-bit ADC count, 0..4095. */
    int y_raw;
    bool axes_valid; /* False on an ADC read failure; raw values then invalid. */
    bool button_pressed; /* Active-low, debounced for 30 ms. */
} micro_input_state_t;

/* Initialize once, then poll from one task about every 10 ms.
 * No LVGL or game dependency. Button state remains valid on ADC failure. */
esp_err_t micro_input_init(void);
esp_err_t micro_input_read(micro_input_state_t *state);
