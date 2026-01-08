#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedRing {
public:
    LedRing(uint8_t pin, uint16_t numPixels);

    void begin();
    void setBrightness(uint8_t brightness);
    void clear();
    void setPixelColor(uint16_t index, uint32_t color);
    void setPixelRgb(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    void show();
    
    uint16_t numPixels() const;
    
    // Utility: pack RGB into uint32_t
    static uint32_t colorRgb(uint8_t r, uint8_t g, uint8_t b);
    // Utility: scale a color by a factor (0.0 - 1.0)
    static uint32_t scaleColor(uint32_t color, float factor);

private:
    Adafruit_NeoPixel _strip;
};
