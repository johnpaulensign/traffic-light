#include "FadeAnimation.h"

void FadeAnimation::onEnter(const AppState& state) {
    (void)state;
    _brightness = 0;
    _increasing = true;
    _lastStepMs = 0;
}

void FadeAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    if (nowMs - _lastStepMs < state.speedMs) return;
    _lastStepMs = nowMs;

    if (_increasing) {
        if (_brightness < 255) _brightness += 5;
        if (_brightness >= 255) { _brightness = 255; _increasing = false; }
    } else {
        if (_brightness > 0) _brightness -= 5;
        if (_brightness == 0) _increasing = true;
    }

    uint32_t color = LedRing::scaleColor(state.primaryColor, _brightness / 255.0f);
    for (uint16_t i = 0; i < ring.numPixels(); i++) {
        ring.setPixelColor(i, color);
    }
    ring.show();
}
