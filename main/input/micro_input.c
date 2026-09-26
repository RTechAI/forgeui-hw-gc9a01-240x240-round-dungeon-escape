#include "micro_input.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

static adc_oneshot_unit_handle_t adc;
static bool initialized;
static bool button_candidate;
static bool button_stable;
static int64_t button_changed_us;

esp_err_t micro_input_init(void)
{
    if (initialized) return ESP_ERR_INVALID_STATE;

    const gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << MICRO_INPUT_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&button_config);
    if (err != ESP_OK) return err;

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    err = adc_oneshot_new_unit(&unit_config, &adc);
    if (err != ESP_OK) return err;

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    /* ESP32-S3 GPIO4 = ADC1_CH3; GPIO5 = ADC1_CH4. */
    err = adc_oneshot_config_channel(adc, ADC_CHANNEL_3, &channel_config);
    if (err == ESP_OK) {
        err = adc_oneshot_config_channel(adc, ADC_CHANNEL_4, &channel_config);
    }
    if (err != ESP_OK) {
        adc_oneshot_del_unit(adc);
        adc = NULL;
        return err;
    }
    button_candidate = gpio_get_level(MICRO_INPUT_BUTTON_GPIO) == 0;
    button_stable = false;
    button_changed_us = esp_timer_get_time();
    initialized = true;
    return ESP_OK;
}

esp_err_t micro_input_read(micro_input_state_t *state)
{
    if (state == NULL) return ESP_ERR_INVALID_ARG;
    *state = (micro_input_state_t){0};
    if (!initialized) return ESP_ERR_INVALID_STATE;

    const bool pressed = gpio_get_level(MICRO_INPUT_BUTTON_GPIO) == 0;
    const int64_t now = esp_timer_get_time();
    if (pressed != button_candidate) {
        button_candidate = pressed;
        button_changed_us = now;
    }
    if (now - button_changed_us >= 30000) button_stable = button_candidate;
    state->button_pressed = button_stable;

    esp_err_t x_err = adc_oneshot_read(adc, ADC_CHANNEL_3, &state->x_raw);
    esp_err_t y_err = adc_oneshot_read(adc, ADC_CHANNEL_4, &state->y_raw);
    state->axes_valid = x_err == ESP_OK && y_err == ESP_OK;
    return x_err != ESP_OK ? x_err : y_err;
}
