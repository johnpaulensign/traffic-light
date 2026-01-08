# teams-ring firmware (`src/`)

## Overview
ESP32 firmware for controlling a WS2812B LED ring and exposing an HTTP API for control.

Hardware assumptions (as currently wired in `main.cpp`):
- LED ring: **12x WS2812B** on **GPIO 38**
- Button: **GPIO 39** (active-low)

The firmware:
- Maintains a shared runtime state (`AppState`).
- Renders animations via a pluggable animation system.
- Exposes a JSON HTTP API (port 80) to control power, brightness, colors, and animation parameters.

## Build / Upload
This is a PlatformIO project.

- Build:
  - `pio run`
- Upload:
  - `pio run -t upload`
- Serial monitor:
  - `pio device monitor`

WiFi credentials are in `src/main.cpp`.

## Project structure
- `main.cpp`
  - Wires everything together (WiFi, HTTP server, button input, animation loop)
- `AppState.h`
  - Shared state used by animations and commands
- `LedRing.h/.cpp`
  - Wrapper around Adafruit NeoPixel
- `AnimationManager.h/.cpp`
  - Registers animations, switches active animation, calls `update()`
- `Commands.h/.cpp`
  - Mutates `AppState` and performs immediate ring actions (power, brightness)
- `HttpApi.h/.cpp`
  - WebServer routes and JSON parsing/serialization
- `ButtonInput.h/.cpp`
  - Debounced button handling with click/hold events
- `animations/`
  - `IAnimation.h` interface and concrete animations

## Animations
Available animation names (query via `GET /animations`):
- `fade`
- `spin`
- `spinTail`
- `strobe`
- `solid`

Notes:
- Animations use `AppState.primaryColor` as the primary color.
- Speed, tail length, and strobe period are configurable through HTTP.

## HTTP API
All endpoints are hosted on port 80.

### Read-only
- `GET /status`
  - Returns JSON including:
    - `powerOn`, `brightness`, `animation`, `color`, `speedMs`, `tailLength`, `strobePeriodMs`, `uptimeMs`
- `GET /animations`
  - Returns a JSON array of animation names.

### Control endpoints
All of these accept either:
- Query params (GET), or
- JSON body (POST)

- `/power`
  - `POST /power` body: `{ "on": true }`
- `/brightness`
  - `POST /brightness` body: `{ "value": 0-255 }`
- `/color`
  - `POST /color` body: `{ "rgb": "#RRGGBB" }`
- `/animation`
  - `POST /animation` body: `{ "name": "fade" }`
- `/speed`
  - `POST /speed` body: `{ "value": <ms> }`
- `/tail`
  - `POST /tail` body: `{ "value": 1-12 }`
- `/strobe`
  - `POST /strobe` body: `{ "value": <periodMs> }`

## Button behavior
Button actions in `main.cpp`:
- Single click: next animation
- Double click: toggle strobe
- Triple click: cycle preset colors
- Hold: toggle power
