#pragma once

#include "IAnimation.h"

class SpinTailAnimation : public IAnimation {
public:
    const char* name() const override { return "spinTail"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    uint32_t _lastStepMs = 0;
    uint16_t _headPosition = 0;
};
