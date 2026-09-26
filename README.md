# ForgeUI Dungeon Escape

ForgeUI Dungeon Escape is an original first-person embedded adventure game built on the ForgeUI Micro Games LVGL 9.2.2 foundation.

The player explores a small dungeon, finds the key, unlocks the exit, and escapes.

## Physical Validation

### Boot screen

![ForgeUI Dungeon Escape boot screen](docs/images/forgeui-dungeon-escape-boot.png)

ForgeUI Dungeon Escape boot screen running on an ESP32-S3 with a GC9A01 round display.

### Level 1 gameplay

![ForgeUI Dungeon Escape Level 1 gameplay](docs/images/forgeui-dungeon-escape-level1.png)

Level 1 dungeon exploration on the GC9A01 240×240 round TFT.

### Key found state

![ForgeUI Dungeon Escape key found state](docs/images/forgeui-dungeon-escape-key-found.png)

The key objective displayed during the Level 1 exploration sequence.

### Door and exit state

![ForgeUI Dungeon Escape door and exit state](docs/images/forgeui-dungeon-escape-door.png)

The locked exit door encountered during the dungeon escape objective.

### Hardware setup

![ForgeUI Dungeon Escape hardware validation](docs/images/forgeui-dungeon-escape-hardware.png)

ForgeUI Dungeon Escape running on ESP32-S3 with GC9A01 round display hardware.

## Technical details

- **Hardware:** ESP32-S3 N16R8
- **Display:** GC9A01 240×240 round TFT
- **Runtime:** LVGL 9.2.2
- **ESP-IDF:** 5.5.4
- **Input:** Analogue joystick GPIO4/GPIO5; button GPIO6

## Features

- First-person raycasting renderer
- Joystick movement
- Collision detection
- Dungeon exploration
- Key collection
- Locked exit
- Escape sequence
- Replay

## Attribution

ForgeUI Dungeon Escape is an original RTechAI ForgeUI Micro Games project.

Third-party components and frameworks:

- [LVGL](https://lvgl.io/)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- [GC9A01 component](https://components.espressif.com/components/atanisoft/esp_lcd_gc9a01)

## Licence

ForgeUI Dungeon Escape is released under the ForgeUI Micro Games Attribution Licence.

This project is intended as an educational embedded development example.

Credit:

ForgeUI Dungeon Escape\
RTechAI / ForgeUI
