#pragma once

#include <Arduino.h>
#include <vector>
#include "AppState.h"
#include "LedRing.h"
#include "animations/IAnimation.h"

class AnimationManager {
public:
    void addAnimation(IAnimation* anim);
    void setActive(const String& name, const AppState& state);
    void nextAnimation(const AppState& state);
    void update(uint32_t nowMs, const AppState& state, LedRing& ring);

    const char* currentName() const;
    std::vector<const char*> listNames() const;

private:
    std::vector<IAnimation*> _animations;
    int _activeIndex = -1;
};
