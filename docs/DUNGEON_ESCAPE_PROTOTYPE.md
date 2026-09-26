# ForgeUI Dungeon Escape — local prototype notes

This local experiment adds a first playable Level 1 to the LVGL 9.2.2 first-person renderer. Public repository identity remains unchanged; no commits or pushes.

## Implemented Level 1 flow

- Move through a maze.
- Find a key.
- Unlock a door.
- Reach the exit.

Arcade boot -> selector -> Dungeon Escape introduction (3 seconds) -> exploration -> key pickup -> return to exit -> unlocking (650 ms) -> enter doorway -> escape effect (900 ms) -> ESCAPED! / LEVEL COMPLETE. A short switch release replays; a one-second hold returns to the selector at any stage.

Key pickup occurs within 0.55 map units, automatically on contact. The locked door is a solid collision tile until the key is collected and the unlock animation finishes. Completion requires entering the exit tile with both the key and open door. Replay and re-entry reset both states.

## Hand-built dungeon

`main/game/dungeon_level.c` owns the readable 12x12 ASCII map: 1=wall, 0=floor, S=start, K=key, D=door. Start at (2.5,4.5), facing south. The exit is behind the start at (2.5,2.5); the key is at (9.5,3.5).

Route for the physical play test: go forward/south into the lower corridor, turn left/east and follow it to its eastern end, then turn left/north into the key room. Touch the gold key. Retrace the corridor west and go north through the start room to the teal exit. With the key, approaching it opens the door; walk through to escape. An optional side room branches south from the main corridor. The HUD direction indicator points directly at the objective, not around walls.

## Implementation boundary

- `main/engine/`: existing camera movement, rotation, collision radius and raycasting math. Added camera accessors, an optional level-tile callback, projection and cached wall depth for objective occlusion. Original renderer map remains the fallback. Old experiment HUD is replaced by the game-owned HUD.
- `main/game/dungeon_level.c`: map, key state, exit state and win condition.
- `main/game/dungeon_escape.c`: lifecycle, intro/completion cards, HUD, primitive world glyphs, messages, door effect, flashes and twelve deterministic sparks.
- Arcade registry uses Dungeon Escape; MicroSnake and Coming Soon are retained.

No assets, full-screen framebuffer, extra tasks, private timers or per-frame LVGL object creation. Objective glyphs are LVGL rectangles clipped against cached wall depth. Animation follows the existing arcade tick, and all objects are released by the existing host screen cleanup.

## Acceptance record

BUILD: PASS (one ESP-IDF 5.5.4 build; image 514,288 bytes, 51% of the existing application partition free)
FLASH: PASS (one flash to COM10; bootloader, partition table and application hashes verified)
PHYSICAL: PENDING (boot, movement/collision, key pickup, locked/unlocked exit, escape screen, replay and hold-to-menu)

One build and one flash/physical play-test session were used. No broad test suite. Boot capture confirms the Dungeon Escape startup, GC9A01 1.2.0, 240x240 LVGL display and 8 MiB PSRAM at 80 MHz. Logs: `build-lvgl922/level1-flash.log` and `build-lvgl922/level1-play.log`.

The three-minute capture stayed at the selector with matching flush/completion counts and no reported watchdog or panic. No level-start, key, unlock or completion events were recorded. Gameplay and visual acceptance therefore remain pending user play-through; build/flash success alone is not a gameplay pass.

## Future possibilities

- Multiple rooms.
- Enemies.
- Pickups.
- Timer.
- Score.

ESP32-S3 N16R8, GC9A01 240×240 round TFT, joystick GPIO4/5/6, ESP-IDF 5.5.4 and LVGL 9.2.2 are retained. Display driver, SPI pins/settings, RGB565 transport, DMA allocation, orientation, PSRAM settings and dependency versions are untouched by Level 1. Future possibilities above remain unimplemented.
