#include "StrobeAnimation.h"

void StrobeAnimation::onEnter(const AppState& state) {
    (void)state;
    _on = false;
    _lastToggleMs = 0;
}

void StrobeAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    uint16_t halfPeriod = state.strobePeriodMs / 2;
    if (halfPeriod == 0) halfPeriod = 50;

    if (nowMs - _lastToggleMs < halfPeriod) return;
    _lastToggleMs = nowMs;

    _on = !_on;
    uint32_t color = _on ? state.primaryColor : 0;
    for (uint16_t i = 0; i < ring.numPixels(); i++) {
        ring.setPixelColor(i, color);
    }
    ring.show();
}
