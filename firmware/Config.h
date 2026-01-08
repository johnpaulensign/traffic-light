#pragma once

#include <Arduino.h>

namespace Config {
    constexpr uint8_t LED_PIN = 38;
    constexpr uint16_t NUM_PIXELS = 3;
    constexpr uint8_t BUTTON_PIN = 41;
    
    // Microsoft Graph API configuration
    // TODO: Replace with your Azure AD app registration values
    constexpr const char* MS_CLIENT_ID = "YOUR_CLIENT_ID_HERE";
    constexpr const char* MS_TENANT_ID = "YOUR_TENANT_ID_HERE";
    
    // Presence polling interval in milliseconds
    constexpr unsigned long PRESENCE_POLL_INTERVAL_MS = 60000;  // 60 seconds
    
    // Strobe duration before transitioning to solid (milliseconds)
    constexpr unsigned long STROBE_DURATION_MS = 1000;
}
