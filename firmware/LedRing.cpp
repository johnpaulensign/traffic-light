#include "LedRing.h"

LedRing::LedRing(uint8_t pin, uint16_t numPixels)
    : _strip(numPixels, pin, NEO_GRB + NEO_KHZ800) {}

void LedRing::begin() {
    _strip.begin();
    _strip.show();
}

void LedRing::setBrightness(uint8_t brightness) {
    _strip.setBrightness(brightness);
}

void LedRing::clear() {
    _strip.clear();
}

void LedRing::setPixelColor(uint16_t index, uint32_t color) {
    _strip.setPixelColor(index, color);
}

void LedRing::setPixelRgb(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    _strip.setPixelColor(index, _strip.Color(r, g, b));
}

void LedRing::show() {
    _strip.show();
}

uint16_t LedRing::numPixels() const {
    return _strip.numPixels();
}

uint32_t LedRing::colorRgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t LedRing::scaleColor(uint32_t color, float factor) {
    if (factor <= 0.0f) return 0;
    if (factor >= 1.0f) return color;
    uint8_t r = ((color >> 16) & 0xFF) * factor;
    uint8_t g = ((color >> 8) & 0xFF) * factor;
    uint8_t b = (color & 0xFF) * factor;
    return colorRgb(r, g, b);
}
