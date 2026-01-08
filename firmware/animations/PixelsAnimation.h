#pragma once

#include "IAnimation.h"

class PixelsAnimation : public IAnimation {
public:
    const char* name() const override { return "pixels"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    uint32_t _lastVersion = 0;
};
