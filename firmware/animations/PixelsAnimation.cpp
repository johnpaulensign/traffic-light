#include "PixelsAnimation.h"
#include "../Config.h"

void PixelsAnimation::onEnter(const AppState& state) {
    (void)state;
    _lastVersion = 0;
}

void PixelsAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    (void)nowMs;

    if (_lastVersion == state.pixelVersion) {
        return;
    }

    const uint16_t n = ring.numPixels();
    for (uint16_t i = 0; i < n && i < Config::NUM_PIXELS; i++) {
        ring.setPixelColor(i, state.pixelColors[i]);
    }
    ring.show();
    _lastVersion = state.pixelVersion;
}
