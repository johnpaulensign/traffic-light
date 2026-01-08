#include "SpinTailAnimation.h"

void SpinTailAnimation::onEnter(const AppState& state) {
    (void)state;
    _headPosition = 0;
    _lastStepMs = 0;
}

void SpinTailAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    if (nowMs - _lastStepMs < state.speedMs) return;
    _lastStepMs = nowMs;

    uint16_t numPixels = ring.numPixels();
    uint8_t tailLen = state.tailLength;
    if (tailLen > numPixels) tailLen = numPixels;

    ring.clear();
    for (uint8_t t = 0; t < tailLen; t++) {
        int16_t idx = (int16_t)_headPosition - t;
        if (idx < 0) idx += numPixels;
        float factor = 1.0f - (float)t / tailLen;
        uint32_t color = LedRing::scaleColor(state.primaryColor, factor);
        ring.setPixelColor((uint16_t)idx, color);
    }
    ring.show();

    _headPosition = (_headPosition + 1) % numPixels;
}
