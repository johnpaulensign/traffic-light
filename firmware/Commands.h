#pragma once

#include "AppState.h"
#include "AnimationManager.h"
#include "LedRing.h"

namespace Commands {

struct PixelUpdate {
    uint16_t position;
    uint32_t color;
};

void togglePower(AppState& state, LedRing& ring);
void setPower(AppState& state, LedRing& ring, bool on);
void setBrightness(AppState& state, LedRing& ring, uint8_t brightness);
void setColor(AppState& state, uint32_t color);

void setColor(AppState& state, uint16_t position, uint32_t color);
void setColors(AppState& state, const PixelUpdate* updates, size_t count);

void setAnimation(AppState& state, AnimationManager& mgr, const String& name);
void nextAnimation(AppState& state, AnimationManager& mgr);
void setSpeed(AppState& state, uint16_t speedMs);
void setTailLength(AppState& state, uint8_t tailLen);
void setStrobePeriod(AppState& state, uint16_t periodMs);

}
