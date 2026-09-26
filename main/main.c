/* ForgeUI Dungeon Escape local prototype, developed by RTechAI. */
#include "arcade/arcade.h"
#include "display/gc9a01.h"
#include "display/display.h"
#include "esp_log.h"
#include "input/micro_input.h"

void app_main(void)
{
    ESP_LOGI("forgeui", "ForgeUI Dungeon Escape Level 1; Flash: 16 MiB; PSRAM: 8 MiB");
    ESP_ERROR_CHECK(micro_input_init());
    gc9a01_displayInit();
    displayConfig(arcade_start);
}
