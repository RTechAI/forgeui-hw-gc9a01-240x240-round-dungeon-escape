/*
 * ForgeUI MicroSnake v0.1, developed by RTechAI.
 * Upstream project attribution is retained in the original source files.
 */
#include "display/gc9a01.h"
#include "display/display.h"
#include "esp_log.h"
#include "input/micro_input.h"

void app_main(void)
{
    ESP_LOGI("forgeui", "ForgeUI MicroSnake v0.1; Flash: 16 MiB; PSRAM: 8 MiB");
    ESP_ERROR_CHECK(micro_input_init());
    gc9a01_displayInit();
    displayConfig();
}
