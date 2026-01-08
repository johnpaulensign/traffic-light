#pragma once

#include <Arduino.h>
#include "../AppState.h"
#include "../LedRing.h"

class IAnimation {
public:
    virtual ~IAnimation() = default;

    virtual const char* name() const = 0;

    // Called when this animation becomes active
    virtual void onEnter(const AppState& state) { (void)state; }

    // Called when switching away from this animation
    virtual void onExit() {}

    // Called every loop iteration; must be non-blocking
    virtual void update(uint32_t nowMs, const AppState& state, LedRing& ring) = 0;
};
