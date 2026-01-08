#include "SolidAnimation.h"

void SolidAnimation::onEnter(const AppState& state) {
    (void)state;
    _needsRefresh = true;
}

void SolidAnimation::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    (void)nowMs;
    
    // Only refresh when entering or if state might have changed
    // For efficiency, we set all pixels once and then just maintain
    if (_needsRefresh) {
        for (uint16_t i = 0; i < ring.numPixels(); i++) {
            ring.setPixelColor(i, state.primaryColor);
        }
        ring.show();
        _needsRefresh = false;
    }
}
