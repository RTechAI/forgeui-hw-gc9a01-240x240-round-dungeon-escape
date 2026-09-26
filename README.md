# ForgeUI MicroDOOM — ESP32-S3 + GC9A01 240×240 Round

ForgeUI MicroDOOM is an experimental ForgeUI Hardware Lab Micro Project developed by RTechAI. It explores Doom-style embedded graphics and rendering experiments on the ESP32-S3 and GC9A01 round display platform.

**Status: project identity and documentation initialized.** This is an experimental graphics project, not a complete Doom port. Doom rendering and gameplay have not been implemented. The inherited firmware remains unchanged, including its existing game modules, selector entries and build identifiers. Building this checkout runs the inherited examples; it does not run MicroDOOM.

## Project context

MicroDOOM follows MicroSnake, MicroPong and MicroAsteroids as part of the ForgeUI Micro Games family. It explores the limits of embedded graphics on small ESP32 hardware, using the ForgeUI Micro Arcade framework as its foundation.

This checkpoint updates documentation and removes copied game evidence images. It adds no rendering or gameplay and makes no MicroDOOM build, performance or physical-validation claims.

## ForgeUI Ecosystem

ForgeUI is developed by [RTechAI](https://github.com/RTechAI).

- [ForgeUI website](https://forgeui.co.nz/)
- [ForgeUI Studio](https://github.com/RTechAI/esp32p4-ui-studio)
- [ForgeUI Hosted Studio](https://studio.forgeui.co.nz/)
- [RTechAI GitHub organisation](https://github.com/RTechAI)
- [ForgeUI Hardware Lab](#forgeui-hardware-lab)

## ForgeUI Hardware Lab

This project uses the physically tested [GC9A01 round-display platform](https://github.com/RTechAI/forgeui-hw-gc9a01-240x240-round) and its shared input/display foundation. The inherited hardware baseline provides a starting point for MicroDOOM experiments. Hardware lineage does not imply Studio target integration or validation of a Doom renderer.

## Hardware reference

**ESP32-S3 N16R8:** 16 MiB Flash, 8 MiB PSRAM.

**GC9A01:** 240×240 round TFT.

| Connection | GPIO |
| --- | --- |
| Display SCLK | 10 |
| Display MOSI | 11 |
| Display DC | 12 |
| Display CS | 13 |
| Display RST | 14 |
| Joystick VRX | 4 |
| Joystick VRY | 5 |
| Joystick SW | 6 |

Use the baseline 3.3 V supply and common ground. No MISO or separate backlight GPIO. Display remains SPI2, mode 0, 20 MHz, RGB565 with LVGL byte swap and existing inversion/mirroring. Joystick input remains ADC1 channels 3/4, 12-bit, 12 dB attenuation, active-low switch and 30 ms debounce.

Flash remains DIO at 80 MHz. PSRAM remains auto-detected octal at 80 MHz, memory-mapped without malloc integration. The baseline boot PSRAM memory test remains disabled to avoid the early watchdog timeout. Display driver, LVGL transport, DMA buffers, LVGL tick/handler task, input driver and board configuration are unchanged.

## Boot and game selector

The preserved ForgeUI Micro Arcade boot experience displays the 2.4-second FORGEUI / MICRO ARCADE ident with animated arc sweeps, teal/cyan colour transitions and orbiting stars. LVGL removes the boot animations when the selector opens.

The registry-based FORGEUI MICRO GAMES selector and shared joystick navigation remain in place. Move up/down, returning to centre between selections, and press to launch an existing module. Hold the button for one second during a running example to return to the selector; release the launch press first. These are inherited framework controls, not MicroDOOM gameplay controls.

## Micro Arcade framework and architecture

```text
main/
  main.c
  arcade/
    arcade.c / arcade.h                 lifecycle and shared input sample
    boot.c / boot.h                     animated console ident
    game_selector.c / game_selector.h   registry-based selector
  games/                               inherited example modules
  input/
    micro_input.c / micro_input.h       shared joystick input
  display/
    display.c / display.h               LVGL transport
    gc9a01.c / gc9a01.h                 panel driver
```

The launcher polls input once every 10 ms on the LVGL task and passes the sample to the active module. Each module implements start/tick/stop. Stop releases module-owned resources; the host deletes screen children. All UI operations run on the existing LVGL task, with startup performed before the task begins.

Module callbacks are registered in `main/arcade/arcade.c`, with sources listed in `main/CMakeLists.txt`. A NULL start marks a coming-soon entry. The framework, existing modules and LVGL setup are preserved for future graphics work.

## Build and flash

Use the baseline ESP-IDF 5.5.4 in an ESP-IDF-enabled terminal. CMake sets the target to `esp32s3`. Tracked `sdkconfig.defaults` supplies board defaults. Dependencies remain LVGL ~8.3.0 and espressif/esp_lcd_gc9a01 ^1.0.

To build and flash the inherited firmware when needed:

```powershell
idf.py -B build-microdoom -p COM10 build flash
```

Replace COM10 if the connected board uses another port. No build or flash was run for this documentation checkpoint. Keep `.vscode/settings.json`, build output, generated files and caches out of commits.

## Images

No MicroDOOM screenshots or hardware evidence images are available yet. Copied game screenshots have been removed from `docs/images`; this README has no image dependencies. Future images should document actual MicroDOOM experiments.

## Attribution

ForgeUI MicroDOOM is developed by RTechAI and built on the ForgeUI Micro Arcade framework. The GC9A01 foundation originated from [UsefulElectronics/esp32s3-gc9a01-lvgl](https://github.com/UsefulElectronics/esp32s3-gc9a01-lvgl). Original Useful Electronics / Ward Almasarani notices are retained. ESP-IDF, LVGL, and the managed GC9A01 component retain their respective licences and notices.
