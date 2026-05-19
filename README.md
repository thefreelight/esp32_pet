# ESP32-C6 Desk Pet

This repository contains a first playable rules-based electronic pet for an ESP32-C6 board.

It is currently set up for the Spotpear 1.47-inch ESP32-C6 display board with:

- ST7789 172x320 LCD
- board backlight control
- board RGB LED
- USB serial input

## Stack

- PlatformIO
- Arduino framework for ESP32
- board LCD as the primary display
- BOOT button as the primary on-device input
- USB serial monitor as the secondary debug input
- Preferences (NVS) for save data
- Chinese on-device UI with pixel-art pet scene

## What You Can Do

- Feed the pet
- Play `STAR CHASE`, a timing mini-game
- Pet it
- Clean the desk
- Send it exploring for random events
- Put it to sleep or wake it up
- Switch between multiple pet species on-device
- Fast-forward time with `skip 15`
- Rename it
- Watch mood and stats on the built-in LCD
- See mood feedback on the built-in RGB LED
- Use a pixel-art animated pet scene instead of a static status page
- Read Chinese action prompts directly on the screen

## Project Layout

- `platformio.ini` configures the ESP32-C6 build
- `src/PetGame.*` contains the game rules and save system
- `src/PetDisplay.*` handles the built-in LCD and RGB LED
- `src/main.cpp` handles USB serial input and app coordination

## Getting Started

1. Create the local tools environment:

   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install platformio
   ```

2. Build and flash:

   ```bash
   pio run -t upload
   ```

3. Open the serial monitor:

   ```bash
   pio device monitor -b 115200
   ```

## First Commands

- `status`
- `feed`
- `play`
- `pet`
- `clean`
- `explore`
- `sleep`
- `wake`
- `skip 15`
- `name mochi`
- `save`
- `new`

## On-Device Controls

- `BOOT` short press: switch to the next action
- `BOOT` hold: perform the selected action
- `RESET/EN`: hardware reset only

This board only gives us one gameplay button on-board: `BOOT`.
`RESET/EN` is a hard reset key, so it is not suitable as a normal game action button.

## STAR CHASE

1. Tap `BOOT` until the selected action becomes `GAME`
2. Hold `BOOT` to start the mini-game
3. Tap or hold `BOOT` when the moving spark reaches the pink heart zone
4. Play 3 rounds and earn joy, bond, and sometimes a discovery

## Pet Roster

1. Tap `BOOT` until the selected action becomes `PETS`
2. Hold `BOOT` to open the roster
3. Tap `BOOT` to browse `CUB CAT`, `LION`, `TIGER`, and `ROBO CAT`
4. Hold `BOOT` to adopt the highlighted pet

Each pet has a slightly different vibe:

- `CUB CAT`: balanced default pet
- `LION`: stronger snack and bonding flavor
- `TIGER`: extra adventure energy
- `ROBO CAT`: original blue robot-cat style pet with cleaner, lower-maintenance behavior

## Next Hardware Upgrade Path

When you are ready, the next step is to add:

- a small OLED or TFT display
- two or three buttons
- a buzzer
- one RGB LED

The game logic is already separated enough that you can keep `PetGame` and swap the serial UI for a real device UI later.

## Screen Board Notes

This build currently assumes the Spotpear board wiring:

- LCD MOSI: `GPIO6`
- LCD SCLK: `GPIO7`
- LCD CS: `GPIO14`
- LCD DC: `GPIO15`
- LCD RST: `GPIO21`
- LCD BL: `GPIO22`
- RGB LED: `GPIO8`
## JARVIS MVP smoke
OnePunch verified this repository through the real GitHub archive, sandbox execution, approval, and pull-request path.
