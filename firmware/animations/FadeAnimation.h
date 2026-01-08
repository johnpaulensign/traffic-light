#pragma once

#include "IAnimation.h"

class FadeAnimation : public IAnimation {
public:
    const char* name() const override { return "fade"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    uint32_t _lastStepMs = 0;
    uint8_t _brightness = 0;
    bool _increasing = true;
};
