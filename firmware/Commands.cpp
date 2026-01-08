#include "Commands.h"

namespace Commands {

void togglePower(AppState& state, LedRing& ring) {
    setPower(state, ring, !state.powerOn);
}

void setPower(AppState& state, LedRing& ring, bool on) {
    state.powerOn = on;
    if (!on) {
        ring.clear();
        ring.show();
    }
}

void setBrightness(AppState& state, LedRing& ring, uint8_t brightness) {
    state.brightness = brightness;
    ring.setBrightness(brightness);
    ring.show();
}

void setColor(AppState& state, uint32_t color) {
    state.primaryColor = color;
}

void setColor(AppState& state, uint16_t position, uint32_t color) {
    if (position >= Config::NUM_PIXELS) return;
    state.pixelColors[position] = color;
    state.pixelVersion++;
}

void setColors(AppState& state, const PixelUpdate* updates, size_t count) {
    bool changed = false;
    for (size_t i = 0; i < count; i++) {
        const uint16_t pos = updates[i].position;
        if (pos >= Config::NUM_PIXELS) continue;
        state.pixelColors[pos] = updates[i].color;
        changed = true;
    }
    if (changed) {
        state.pixelVersion++;
    }
}

void setAnimation(AppState& state, AnimationManager& mgr, const String& name) {
    state.currentAnimationName = name;
    mgr.setActive(name, state);
}

void nextAnimation(AppState& state, AnimationManager& mgr) {
    mgr.nextAnimation(state);
    state.currentAnimationName = mgr.currentName();
}

void setSpeed(AppState& state, uint16_t speedMs) {
    state.speedMs = speedMs;
}

void setTailLength(AppState& state, uint8_t tailLen) {
    state.tailLength = tailLen;
}

void setStrobePeriod(AppState& state, uint16_t periodMs) {
    state.strobePeriodMs = periodMs;
}

}
