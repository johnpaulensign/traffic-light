#include "AnimationManager.h"

void AnimationManager::addAnimation(IAnimation* anim) {
    _animations.push_back(anim);
}

void AnimationManager::setActive(const String& name, const AppState& state) {
    for (size_t i = 0; i < _animations.size(); i++) {
        if (name.equalsIgnoreCase(_animations[i]->name())) {
            if (_activeIndex >= 0 && _activeIndex < (int)_animations.size()) {
                _animations[_activeIndex]->onExit();
            }
            _activeIndex = (int)i;
            _animations[_activeIndex]->onEnter(state);
            return;
        }
    }
}

void AnimationManager::nextAnimation(const AppState& state) {
    if (_animations.empty()) return;
    if (_activeIndex >= 0 && _activeIndex < (int)_animations.size()) {
        _animations[_activeIndex]->onExit();
    }
    _activeIndex = (_activeIndex + 1) % (int)_animations.size();
    _animations[_activeIndex]->onEnter(state);
}

void AnimationManager::update(uint32_t nowMs, const AppState& state, LedRing& ring) {
    if (_activeIndex >= 0 && _activeIndex < (int)_animations.size()) {
        _animations[_activeIndex]->update(nowMs, state, ring);
    }
}

const char* AnimationManager::currentName() const {
    if (_activeIndex >= 0 && _activeIndex < (int)_animations.size()) {
        return _animations[_activeIndex]->name();
    }
    return "";
}

std::vector<const char*> AnimationManager::listNames() const {
    std::vector<const char*> names;
    for (auto* a : _animations) {
        names.push_back(a->name());
    }
    return names;
}
