#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "AppState.h"
#include "AnimationManager.h"
#include "LedRing.h"
#include "Commands.h"

class HttpApi {
public:
    HttpApi(AppState& state, AnimationManager& mgr, LedRing& ring, uint16_t port = 80);

    void begin();
    void poll();

private:
    AppState& _state;
    AnimationManager& _mgr;
    LedRing& _ring;
    WebServer _server;

    void handleStatus();
    void handleAnimations();
    void handleSetAnimation();
    void handleSetBrightness();
    void handleSetColor();
    void handleSetPixel();
    void handleSetPixels();
    void handleSetPower();
    void handleSetSpeed();
    void handleSetTail();
    void handleSetStrobe();

    void sendOk();
    void sendError(const String& msg);
    uint32_t parseColor(const String& str);
};
