# ForgeUI MicroSnake — ESP32-S3 + GC9A01 240×240 Round

ForgeUI MicroSnake is an official ForgeUI Hardware Lab Micro Project developed by RTechAI. It is a joystick-controlled embedded game built on the physically tested ESP32-S3 + GC9A01 240×240 round display baseline.

**Project status:** This initial release establishes the MicroSnake project identity and documentation. Joystick control and gameplay are planned; the current firmware retains the proven ForgeUI ALIVE display showcase.

## ForgeUI Ecosystem

ForgeUI is developed by [RTechAI](https://github.com/RTechAI). The [ForgeUI website](https://forgeui.co.nz/) introduces the platform, while [ForgeUI Studio](https://github.com/RTechAI/esp32p4-ui-studio) is the public visual embedded UI/HMI development environment for supported ESP32 hardware.

[ForgeUI Hosted Studio](https://studio.forgeui.co.nz/) is the browser-based ForgeUI Studio application for visual design, native LVGL C generation, browser preview, and supported ESP-IDF build-and-flash workflows. The [RTechAI GitHub organisation](https://github.com/RTechAI) hosts ForgeUI public repositories, hardware references, framework baselines, examples, and related development work.

[ForgeUI Hardware Lab](#forgeui-hardware-lab) is the physically tested hardware and project collection within the ForgeUI ecosystem. MicroSnake is its first official ForgeUI Micro Project.

```text
RTechAI
└── ForgeUI
    └── ForgeUI Hardware Lab
        └── Micro Projects
            └── MicroSnake
```

## Micro Project Overview

MicroSnake brings embedded game development to a small round display. Its planned joystick-controlled Snake game provides a practical way to explore ESP32-S3 graphics, LVGL rendering, input handling, and game logic.

The project is intended for learning, experimentation, and hacking. Future expansion can introduce gameplay modes, animation, scoring, and other small-screen experiments on the proven hardware foundation.

The Micro Project concept:

> Flash it.<br>
> Play it.<br>
> Open the code.<br>
> Hack it.

For now, flashing runs the inherited ALIVE showcase; playable MicroSnake will follow in a later development stage.

## Hardware Foundation

MicroSnake builds on the physically tested [ForgeUI GC9A01 round-display baseline](https://github.com/RTechAI/forgeui-hw-gc9a01-240x240-round).

- ESP32-S3 DevKitC-1 N16R8
- Flash: 16 MiB
- PSRAM: 8 MiB
- Display: GC9A01 1.28-inch 240×240 round SPI TFT
- No MISO connection and no separate backlight pin

The baseline validated firmware build and flash, GC9A01 initialization and SPI rendering, LVGL startup, the ForgeUI ALIVE showcase, and GPIO10–GPIO14 flat-ribbon wiring. The evidence below records that hardware validation, not MicroSnake gameplay validation.

## Display Wiring

| GC9A01 | ESP32-S3 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SCL / SCLK | GPIO10 |
| SDA / MOSI | GPIO11 |
| DC | GPIO12 |
| CS | GPIO13 |
| RST | GPIO14 |
| MISO | Unused |

GPIO10–GPIO14 are the physically proven flat-ribbon display connection. Do not change them. Joystick wiring is not defined at this stage.

## Software Stack

- ESP-IDF 5.5.4
- LVGL 8.3.11
- `espressif/esp_lcd_gc9a01` 1.2.0
- SPI2, mode 0, 20 MHz, RGB565 with `CONFIG_LV_COLOR_16_SWAP=y`
- 16 MiB DIO flash at 80 MHz
- 8 MiB auto-detected octal PSRAM at 80 MHz DDR
- `CONFIG_SPIRAM_USE_MEMMAP=y`: PSRAM is initialized and mapped, without malloc-heap integration

The firmware currently includes the lightweight LVGL-only ForgeUI ALIVE showcase: an animated circular readiness ring and the display/platform identity. The display driver, LVGL runtime, ESP-IDF configuration, PSRAM configuration, build files, and dependencies are retained from the hardware foundation.

## Build and Flash

Use ESP-IDF 5.5.4 with the board connected:

```powershell
idf.py build
idf.py -p COMx flash
idf.py -p COMx monitor
```

Replace `COMx` with the detected port. Use `Ctrl-]` to exit the monitor. These commands run the current ALIVE showcase firmware.

## Hardware Foundation Evidence

| Image | Evidence |
| --- | --- |
| [gc9a01-round-alive-boot.png](docs/images/gc9a01-round-alive-boot.png) | ForgeUI Hardware Lab startup sequence on the ESP32-S3 N16R8. |
| [gc9a01-round-alive-showcase.png](docs/images/gc9a01-round-alive-showcase.png) | Animated ForgeUI GC9A01 LVGL ALIVE showcase. |
| [gc9a01-round-hardware-validation.png](docs/images/gc9a01-round-hardware-validation.png) | Physical GC9A01 240×240 round-display hardware evidence. |

![ForgeUI GC9A01 hardware foundation ALIVE showcase](docs/images/gc9a01-round-alive-showcase.png)

## Related ForgeUI Projects

- [GC9A01 Round Display Baseline](https://github.com/RTechAI/forgeui-hw-gc9a01-240x240-round) — MicroSnake's physically tested hardware foundation.
- [ForgeUI-P4](https://github.com/RTechAI/ForgeUI-P4) — ESP32-P4 LVGL hardware baseline and framework.
- [esp32p4-ui-studio](https://github.com/RTechAI/esp32p4-ui-studio) — public local ForgeUI Studio reference for ESP32-P4 and LVGL 9.
- [ForgeUI-One](https://github.com/RTechAI/ForgeUI-One) — ESP32-P4 LVGL starter baseline and embedded UI framework.

## ForgeUI Hardware Lab

ForgeUI Hardware Lab is an RTechAI collection of physically tested ESP32 boards, displays, peripherals, examples, and experimental projects. Each project documents hardware identity, wiring configuration, software baseline, reproducible build process, and physical validation evidence.

Micro Projects build focused applications on these proven foundations. MicroSnake is an official ForgeUI Hardware Lab Micro Project developed by RTechAI. Its hardware lineage does not imply current ForgeUI Studio target integration.

## Attribution

The MicroSnake application/project identity belongs to ForgeUI/RTechAI. Its hardware foundation comes from [forgeui-hw-gc9a01-240x240-round](https://github.com/RTechAI/forgeui-hw-gc9a01-240x240-round), whose upstream history and references remain credited.

That baseline began from [UsefulElectronics/esp32s3-gc9a01-lvgl](https://github.com/UsefulElectronics/esp32s3-gc9a01-lvgl). Its unrelated application code and generated UI assets were removed from the active baseline. Original Useful Electronics / Ward Almasarani attribution remains in the retained GC9A01 header.

ESP-IDF, LVGL, and the managed `espressif/esp_lcd_gc9a01` driver component remain subject to their own licences and notices. Their attribution and retained source notices are preserved.
