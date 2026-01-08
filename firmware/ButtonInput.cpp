#include "ButtonInput.h"

ButtonInput::ButtonInput(uint8_t pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow) {}

void ButtonInput::begin() {
    pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
}

bool ButtonInput::readRaw() {
    bool raw = digitalRead(_pin);
    return _activeLow ? !raw : raw;
}

ButtonEvent ButtonInput::update(uint32_t nowMs) {
    bool raw = readRaw();
    ButtonEvent event = ButtonEvent::None;

    // Debounce
    if (raw != _lastRawState) {
        _lastDebounceMs = nowMs;
        _lastRawState = raw;
    }

    bool debounced = _stableState;
    if ((nowMs - _lastDebounceMs) >= DEBOUNCE_MS) {
        debounced = _lastRawState;
    }

    bool wasPressed = _stableState;
    bool isPressed = debounced;

    // Detect transitions
    if (isPressed && !wasPressed) {
        // Press down
        _pressStartMs = nowMs;
        _holdFired = false;
    } else if (!isPressed && wasPressed) {
        // Release
        if (!_holdFired) {
            _clickCount++;
            _lastReleaseMs = nowMs;
        }
        _holdFired = false;
    }

    // Hold detection (while still pressed)
    if (isPressed && !_holdFired) {
        if ((nowMs - _pressStartMs) >= HOLD_THRESHOLD_MS) {
            _holdFired = true;
            _clickCount = 0; // Cancel any pending clicks
            event = ButtonEvent::Hold;
        }
    }

    // Multi-click window expired?
    if (!isPressed && _clickCount > 0) {
        if ((nowMs - _lastReleaseMs) >= MULTI_CLICK_WINDOW_MS) {
            if (_clickCount == 1) event = ButtonEvent::Click1;
            else if (_clickCount == 2) event = ButtonEvent::Click2;
            else if (_clickCount >= 3) event = ButtonEvent::Click3;
            _clickCount = 0;
        }
    }

    _stableState = debounced;
    return event;
}
