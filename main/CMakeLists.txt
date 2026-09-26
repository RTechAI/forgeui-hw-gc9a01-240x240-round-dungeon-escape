idf_component_register(
    SRCS "main.c" "display/display.c" "display/gc9a01.c"
         "input/micro_input.c" "arcade/arcade.c" "arcade/game_selector.c" "arcade/boot.c"
         "games/microsnake/microsnake.c" "engine/dungeon_camera.c" "engine/dungeon_renderer.c"
         "game/dungeon_level.c" "game/dungeon_escape.c"
    INCLUDE_DIRS ".")
