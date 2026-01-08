#pragma once

#include <Arduino.h>
#include "Config.h"

struct AppState {
    bool powerOn = true;
    uint8_t brightness = 128;
    uint32_t primaryColor = 0x0000FF;   // Blue
    uint32_t secondaryColor = 0x000000; // Off
    String currentAnimationName = "fade";
    uint16_t speedMs = 50;              // Base step interval in ms
    
    // Animation-specific params
    uint8_t tailLength = 6;             // For spin-tail
    uint16_t strobePeriodMs = 100;      // For strobe (on+off cycle)

    uint32_t pixelColors[Config::NUM_PIXELS] = {0};
    uint32_t pixelVersion = 1;
};
