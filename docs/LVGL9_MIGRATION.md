# LVGL 8.3.11 to 9.2.2 migration

Historical migration note: this audit was completed before the project was renamed from MicroDOOM to ForgeUI Dungeon Escape.

## Findings and adaptations

- Display: `lv_disp_drv_t`, `lv_disp_draw_buf_t`, `lv_disp_draw_buf_init`, `lv_disp_drv_init`, `lv_disp_drv_register`, driver fields and `lv_disp_flush_ready` are replaced by `lv_display_t`, creation/setter APIs and `lv_display_flush_ready`.
- Buffers remain two DMA allocations of 240 x 20 x 2 bytes (9,600 bytes each). LVGL 9 buffer sizes are bytes; `lv_color_t` is no longer the RGB565 storage type. Set RGB565 explicitly and use partial rendering.
- Flush receives `uint8_t *`; user data comes from `lv_display_get_user_data`. Swap RGB565 once before SPI submission, replacing removed `CONFIG_LV_COLOR_16_SWAP`. Signal completion only from the panel transfer callback. Panel IO retains the address of the display pointer because it is created before LVGL.
- Historical MicroDOOM custom drawing: replace `lv_draw_ctx_t` / `lv_event_get_draw_ctx` with `lv_layer_t` / `lv_event_get_layer`. Keep rectangle descriptors, current-strip clipping and draw order. Raycasting, palette, geometry, collision and movement are unchanged.
- Arcade and games: `lv_scr_act` becomes `lv_screen_active`, `lv_obj_clear_flag` becomes `lv_obj_remove_flag`, and `lv_anim_set_time` becomes `lv_anim_set_duration`. Other object, style, arc, label, color, event, tick and timer APIs retain their use.
- Inherited MicroAsteroids source also uses old draw contexts and the four-argument `lv_draw_line`. Adapt to layers and descriptor endpoints (`lv_point_precise_t`). It remains excluded from CMake and the selector, so the firmware build does not validate this dormant module.
- Configuration: replace LVGL 8 configuration with LVGL 9 settings; retain RGB565, 30 ms refresh, 32 KiB built-in LVGL heap, Montserrat 14 and software rendering. Explicitly enable complex drawing for boot arcs. Retain all non-LVGL board/PSRAM settings.
- Dependencies: pin LVGL 9.2.2, ESP-IDF 5.5.4 and GC9A01 1.2.0. Let the component manager regenerate the lockfile.
- Copied build caches reference the old folder. Use fresh `build-lvgl922` without deleting the previous outputs.

## Preserved behavior and hardware

The 2.4-second animated ForgeUI Micro Arcade boot, selector, input polling and hold-to-menu behavior were preserved through the rename. MicroAsteroids exists only as inherited source.

Display GPIO10/11/12/13/14, joystick GPIO4/5/6, SPI2 mode 0 at 20 MHz, DMA, RGB565/BGR panel setting, inversion and mirroring, 16 MiB DIO flash and mapped octal 80 MHz PSRAM remain unchanged.

## Validation

The original build and flash/physical checks were completed. Scott then explicitly authorized one additional build-and-flash cycle to verify the watchdog fix. No broad tests, commits or pushes.

BUILD: PASS using ESP-IDF 5.5.4; LVGL 9.2.2 and GC9A01 1.2.0 resolved. Final image is 494,400 bytes, leaving 53% of the existing 1 MiB app partition free.
FLASH: PASS on COM10; bootloader, partition table and application hashes verified.
STARTUP: PASS over the captured 20 seconds. GC9A01 and 8 MiB PSRAM at 80 MHz initialize; 240x240 RGB565 display uses two 20-row DMA buffers. Flush and completion counts match. No task-watchdog warnings in the final capture.
PHYSICAL: PASS. The final Dungeon Escape image was clean-built, flashed, booted and physically validated through selector launch, Level 1, key pickup, exit unlock and escape sequence.

Evidence: `build-lvgl922/flash-boot.log` (final image); build logs under `build-lvgl922/log/`.

## Complete pre-migration LVGL symbol inventory

Includes types and symbols used by active and dormant application sources. All were reviewed; migration-specific incompatibilities are listed above.

```text
lv_anim_exec_xcb_t
lv_anim_init
lv_anim_set_exec_cb
lv_anim_set_time
lv_anim_set_values
lv_anim_set_var
lv_anim_start
lv_anim_t
lv_arc_create
lv_arc_set_bg_angles
lv_arc_set_rotation
lv_area_t
lv_color_black
lv_color_hex
lv_color_make
lv_color_mix
lv_color_t
lv_coord_t
lv_disp_draw_buf_init
lv_disp_draw_buf_t
lv_disp_drv_init
lv_disp_drv_register
lv_disp_drv_t
lv_disp_flush_ready
lv_draw_ctx_t
lv_draw_line
lv_draw_line_dsc_init
lv_draw_line_dsc_t
lv_draw_rect
lv_draw_rect_dsc_init
lv_draw_rect_dsc_t
lv_event_get_draw_ctx
lv_event_get_target
lv_event_t
lv_font_montserrat_14
lv_init
lv_label_create
lv_label_set_text
lv_label_set_text_fmt
lv_obj_add_event_cb
lv_obj_align
lv_obj_center
lv_obj_clean
lv_obj_clear_flag
lv_obj_create
lv_obj_get_coords
lv_obj_invalidate
lv_obj_remove_style_all
lv_obj_set_pos
lv_obj_set_size
lv_obj_set_style_arc_color
lv_obj_set_style_arc_opa
lv_obj_set_style_arc_rounded
lv_obj_set_style_arc_width
lv_obj_set_style_bg_color
lv_obj_set_style_bg_opa
lv_obj_set_style_border_color
lv_obj_set_style_border_width
lv_obj_set_style_opa
lv_obj_set_style_pad_hor
lv_obj_set_style_pad_ver
lv_obj_set_style_radius
lv_obj_set_style_text_align
lv_obj_set_style_text_color
lv_obj_set_style_text_font
lv_obj_set_style_text_letter_space
lv_obj_set_width
lv_obj_t
lv_point_t
lv_scr_act
lv_tick_elaps
lv_tick_get
lv_tick_inc
lv_timer_create
lv_timer_handler
lv_timer_t
```

## Runtime issue resolved

The first flash exposed IDLE0 task-watchdog starvation at 5, 10 and 15 seconds. The inherited `vTaskDelay(pdMS_TO_TICKS(5))` rounds to zero at `CONFIG_FREERTOS_HZ=100`. The handler now clamps its delay to at least one tick, preserving the OS tick configuration, LVGL tick timer, task priority and stack size. The user-approved additional build/flash passed and the final 20-second boot capture contains no watchdog warnings.

## User observation

After the first flash, Scott reported "yes seem sok" in response to the combined boot, selector, MicroDOOM rendering/movement/collision, MicroSnake and hold-to-menu check. This historical observation preceded the final Dungeon Escape physical-validation pass.

## Files changed

- main/idf_component.yml
- main/display/display.c
- main/display/display.h
- main/display/gc9a01.c
- main/engine/dungeon_renderer.c
- main/engine/dungeon_camera.c
- main/arcade/arcade.c
- main/arcade/boot.c
- main/arcade/game_selector.c
- main/games/microsnake/microsnake.c
- main/games/microasteroids/microasteroids.c (dormant module)
- sdkconfig
- sdkconfig.defaults
- dependencies.lock (component-manager generated)
- README.md
- docs/LVGL9_MIGRATION.md

Managed LVGL sources and build outputs were regenerated by the dependency manager/build. ESP-IDF also refreshed sdkconfig.old. No commits or pushes were performed.
