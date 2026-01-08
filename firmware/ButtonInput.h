#pragma once

#include <Arduino.h>

enum class ButtonEvent {
    None,
    Click1,
    Click2,
    Click3,
    Hold
};

class ButtonInput {
public:
    ButtonInput(uint8_t pin, bool activeLow = true);

    void begin();
    ButtonEvent update(uint32_t nowMs);

private:
    uint8_t _pin;
    bool _activeLow;

    // Debounce
    bool _lastRawState = false;
    bool _stableState = false;
    uint32_t _lastDebounceMs = 0;
    static constexpr uint32_t DEBOUNCE_MS = 30;

    // Click counting
    uint8_t _clickCount = 0;
    uint32_t _lastReleaseMs = 0;
    static constexpr uint32_t MULTI_CLICK_WINDOW_MS = 300;

    // Hold detection
    uint32_t _pressStartMs = 0;
    bool _holdFired = false;
    static constexpr uint32_t HOLD_THRESHOLD_MS = 800;

    bool readRaw();
};
