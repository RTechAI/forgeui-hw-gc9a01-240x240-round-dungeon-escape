idf_component_register(
    SRCS "main.c" "display/display.c" "display/gc9a01.c"
         "input/micro_input.c" "arcade/arcade.c" "arcade/game_selector.c" "arcade/boot.c"
         "games/microsnake/microsnake.c" "engine/microdoom.c" "engine/renderer.c"
    INCLUDE_DIRS ".")
