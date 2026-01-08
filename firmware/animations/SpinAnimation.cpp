#include "SpinAnimation.h"

void SpinAnimation::onEnter(const AppState& state) {
    (void)state;
    _position = 0;
    _lastStepMs = 0;
}

void SpinAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    if (nowMs - _lastStepMs < state.speedMs) return;
    _lastStepMs = nowMs;

    ring.clear();
    ring.setPixelColor(_position, state.primaryColor);
    ring.show();

    _position = (_position + 1) % ring.numPixels();
}
