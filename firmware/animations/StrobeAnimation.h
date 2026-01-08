#pragma once

#include "IAnimation.h"

class StrobeAnimation : public IAnimation {
public:
    const char* name() const override { return "strobe"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    uint32_t _lastToggleMs = 0;
    bool _on = false;
};
