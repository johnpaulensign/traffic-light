#pragma once

#include "IAnimation.h"

class SolidAnimation : public IAnimation {
public:
    const char* name() const override { return "solid"; }
    void onEnter(const AppState& state) override;
    void update(uint32_t nowMs, const AppState& state, LedRing& ring) override;

private:
    bool _needsRefresh = true;
};
