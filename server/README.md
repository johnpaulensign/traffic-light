# teams-ring director (`server/`)

## Overview
Python "director" service that:
- Authenticates to Microsoft Graph (device code flow)
- Polls Teams presence (`availability`)
- Controls the ESP32 LED ring via HTTP endpoints

Desired effects:
- **Green** (Available): solid green
- **Yellow** (Away / BeRightBack): fade in/out
- **Red** (Busy / InAMeeting / DoNotDisturb / etc.): strobe for ~1s, then solid red

## Files
- `director.py`
  - Main loop and state machine
  - Polls Teams + applies the appropriate effect
- `teams_client.py`
  - MSAL auth + `get_presence()`
- `esp32_client.py`
  - Typed wrapper for ESP32 endpoints (JSON POST)
- `effects.py`
  - Presence -> effect mapping
- `config.py`
  - Loads `settings.json`
- `test.http`
  - HTTP requests you can run from the IDE to test ESP32 endpoints
- `requirements.txt`
  - Python dependencies

## Setup
Install dependencies:

- `pip install -r requirements.txt`

## Configuration
Create/update `server/settings.json`:

```json
{
  "client_id": "<azure app client id>",
  "tenant_id": "<azure tenant id>",
  "esp32_host": "http://<esp32 ip>",
  "refresh_interval_seconds": 5,
  "strobe_duration_seconds": 1.0,
  "strobe_period_ms": 100,
  "fade_speed_ms": 40,
  "mode": "ring"
}
```

Only `client_id`, `tenant_id`, and `esp32_host` are required.

## Display Modes

The `mode` setting controls how the director displays Teams presence:

### `ring` (default)
Uses the full LED ring with animations:
- **Available**: Solid green (all LEDs)
- **Away/BeRightBack**: Fading yellow
- **Busy/DoNotDisturb/InAMeeting**: Strobe red for 1s, then solid red
- **Offline**: LEDs off

### `trafficlight`
Uses 3 LEDs as a traffic light (one lit at a time):
- **Position 0** (bottom): Green LED
- **Position 1** (middle): Yellow LED
- **Position 2** (top): Red LED

Presence mapping:
- **Available**: Green LED on, others off
- **Away/BeRightBack**: Yellow LED on, others off
- **Busy/DoNotDisturb/InAMeeting**: Red LED on, others off
- **Offline**: All LEDs off

To use traffic light mode, set `"mode": "trafficlight"` in `settings.json`.

## Running
Run from the **project root** (so `server` is an importable package):

- `python -m server`

On first run you will get a device login prompt. Follow the URL and enter the code.

## Logging / failures
- ESP32 requests use a **3 second timeout**.
- If the ESP32 is down/restarting and a request fails, the director logs a **warning**.

## ESP32 state checking
Before applying an effect, the director calls `GET /status` and compares:
- `powerOn`
- `color`
- `animation`

If the ESP32 already matches the desired state, it will skip re-sending commands.

## Testing the ESP32 manually
Open and run requests from:
- `server/test.http`

Update the `@host` variable at the top to match your ESP32 IP.
