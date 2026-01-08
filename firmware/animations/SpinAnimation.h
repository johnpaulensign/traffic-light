#pragma once

#include "IAnimation.h"

class SpinAnimation : public IAnimation {
public:
    const char* name() const override { return "spin"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    uint32_t _lastStepMs = 0;
    uint16_t _position = 0;
};
