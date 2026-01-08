#pragma once

#include <Arduino.h>
#include "MicrosoftAuth.h"

enum class Presence {
    Available,
    Away,
    BeRightBack,
    Busy,
    DoNotDisturb,
    InACall,
    InAMeeting,
    Presenting,
    Offline,
    Unknown
};

enum class EffectType {
    Solid,
    Pixel,
    StrobeThenPixel,
    Fade,
    StrobeThenSolid,
    Off
};

enum class TrafficLightState {
    Bottom,
    Middle,
    Top,
    All
};

struct PresenceEffect {
    EffectType type;
    uint32_t color;
    TrafficLightState trafficLight;
};

class TeamsPresence {
public:
    TeamsPresence(MicrosoftAuth& auth);
    
    bool fetchPresence();
    
    Presence getPresence() const { return _presence; }
    const char* getPresenceString() const;
    
    PresenceEffect getEffect() const;
    
    static PresenceEffect mapPresenceToEffect(Presence presence);
    
private:
    MicrosoftAuth& _auth;
    Presence _presence;
    
    Presence parsePresence(const String& availability);
};
