#include "HttpApi.h"

HttpApi::HttpApi(AppState& state, AnimationManager& mgr, LedRing& ring, uint16_t port)
    : _state(state), _mgr(mgr), _ring(ring), _server(port) {}

void HttpApi::begin() {
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/animations", HTTP_GET, [this]() { handleAnimations(); });
    _server.on("/animation", HTTP_GET, [this]() { handleSetAnimation(); });
    _server.on("/animation", HTTP_POST, [this]() { handleSetAnimation(); });
    _server.on("/brightness", HTTP_GET, [this]() { handleSetBrightness(); });
    _server.on("/brightness", HTTP_POST, [this]() { handleSetBrightness(); });
    _server.on("/color", HTTP_GET, [this]() { handleSetColor(); });
    _server.on("/color", HTTP_POST, [this]() { handleSetColor(); });
    _server.on("/pixel", HTTP_GET, [this]() { handleSetPixel(); });
    _server.on("/pixel", HTTP_POST, [this]() { handleSetPixel(); });
    _server.on("/pixels", HTTP_POST, [this]() { handleSetPixels(); });
    _server.on("/power", HTTP_GET, [this]() { handleSetPower(); });
    _server.on("/power", HTTP_POST, [this]() { handleSetPower(); });
    _server.on("/speed", HTTP_GET, [this]() { handleSetSpeed(); });
    _server.on("/speed", HTTP_POST, [this]() { handleSetSpeed(); });
    _server.on("/tail", HTTP_GET, [this]() { handleSetTail(); });
    _server.on("/tail", HTTP_POST, [this]() { handleSetTail(); });
    _server.on("/strobe", HTTP_GET, [this]() { handleSetStrobe(); });
    _server.on("/strobe", HTTP_POST, [this]() { handleSetStrobe(); });
    _server.begin();
}

void HttpApi::poll() {
    _server.handleClient();
}

void HttpApi::handleStatus() {
    StaticJsonDocument<512> doc;
    doc["powerOn"] = _state.powerOn;
    doc["brightness"] = _state.brightness;
    doc["animation"] = _mgr.currentName();
    char colorHex[8];
    snprintf(colorHex, sizeof(colorHex), "#%06X", (unsigned int)_state.primaryColor);
    doc["color"] = colorHex;
    doc["speedMs"] = _state.speedMs;
    doc["tailLength"] = _state.tailLength;
    doc["strobePeriodMs"] = _state.strobePeriodMs;
    doc["uptimeMs"] = millis();

    String out;
    serializeJson(doc, out);
    _server.send(200, "application/json", out);
}

void HttpApi::handleAnimations() {
    StaticJsonDocument<256> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto* name : _mgr.listNames()) {
        arr.add(name);
    }
    String out;
    serializeJson(doc, out);
    _server.send(200, "application/json", out);
}

void HttpApi::handleSetAnimation() {
    String name;
    if (_server.hasArg("name")) {
        name = _server.arg("name");
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            name = doc["name"] | "";
        }
    }
    if (name.length() == 0) {
        sendError("Missing 'name'");
        return;
    }
    Commands::setAnimation(_state, _mgr, name);
    sendOk();
}

void HttpApi::handleSetBrightness() {
    int val = -1;
    if (_server.hasArg("value")) {
        val = _server.arg("value").toInt();
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            val = doc["value"] | -1;
        }
    }
    if (val < 0 || val > 255) {
        sendError("Invalid 'value' (0-255)");
        return;
    }
    Commands::setBrightness(_state, _ring, (uint8_t)val);
    sendOk();
}

void HttpApi::handleSetColor() {
    String colorStr;
    if (_server.hasArg("rgb")) {
        colorStr = _server.arg("rgb");
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            colorStr = doc["rgb"] | "";
        }
    }
    if (colorStr.length() == 0) {
        sendError("Missing 'rgb'");
        return;
    }
    uint32_t color = parseColor(colorStr);
    Commands::setColor(_state, color);
    sendOk();
}

void HttpApi::handleSetPixel() {
    int pos = -1;
    String colorStr;

    if (_server.hasArg("position")) {
        pos = _server.arg("position").toInt();
    }
    if (_server.hasArg("rgb")) {
        colorStr = _server.arg("rgb");
    }

    if (_server.hasArg("plain")) {
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            pos = doc["position"] | pos;
            colorStr = doc["rgb"] | colorStr;
        }
    }

    if (pos < 0 || pos >= (int)Config::NUM_PIXELS) {
        sendError("Invalid 'position' (0-11)");
        return;
    }
    if (colorStr.length() == 0) {
        sendError("Missing 'rgb'");
        return;
    }

    uint32_t color = parseColor(colorStr);
    Commands::setColor(_state, (uint16_t)pos, color);
    Commands::setAnimation(_state, _mgr, "pixels");
    sendOk();
}

void HttpApi::handleSetPixels() {
    if (!_server.hasArg("plain")) {
        sendError("Missing JSON body");
        return;
    }

    StaticJsonDocument<768> doc;
    if (deserializeJson(doc, _server.arg("plain")) != DeserializationError::Ok) {
        sendError("Invalid JSON");
        return;
    }

    JsonArray arr;
    if (doc.is<JsonArray>()) {
        arr = doc.as<JsonArray>();
    } else if (doc.is<JsonObject>() && doc.containsKey("pixels") && doc["pixels"].is<JsonArray>()) {
        arr = doc["pixels"].as<JsonArray>();
    } else {
        sendError("Expected JSON array or {\"pixels\": [...]} ");
        return;
    }

    Commands::PixelUpdate updates[Config::NUM_PIXELS];
    size_t count = 0;

    for (JsonVariant v : arr) {
        if (!v.is<JsonObject>()) continue;
        JsonObject o = v.as<JsonObject>();
        int pos = o["position"] | -1;
        String rgb = o["rgb"] | "";
        if (pos < 0 || pos >= (int)Config::NUM_PIXELS) continue;
        if (rgb.length() == 0) continue;

        if (count >= Config::NUM_PIXELS) break;
        updates[count].position = (uint16_t)pos;
        updates[count].color = parseColor(rgb);
        count++;
    }

    if (count == 0) {
        sendError("No valid pixels provided");
        return;
    }

    Commands::setColors(_state, updates, count);
    Commands::setAnimation(_state, _mgr, "pixels");
    sendOk();
}

void HttpApi::handleSetPower() {
    int on = -1;
    if (_server.hasArg("on")) {
        String s = _server.arg("on");
        if (s == "true" || s == "1") on = 1;
        else if (s == "false" || s == "0") on = 0;
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            if (doc.containsKey("on")) {
                on = doc["on"].as<bool>() ? 1 : 0;
            }
        }
    }
    if (on < 0) {
        sendError("Missing 'on' (true/false)");
        return;
    }
    Commands::setPower(_state, _ring, on == 1);
    sendOk();
}

void HttpApi::handleSetSpeed() {
    int val = -1;
    if (_server.hasArg("value")) {
        val = _server.arg("value").toInt();
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            val = doc["value"] | -1;
        }
    }
    if (val < 1) {
        sendError("Invalid 'value' (>0)");
        return;
    }
    Commands::setSpeed(_state, (uint16_t)val);
    sendOk();
}

void HttpApi::handleSetTail() {
    int val = -1;
    if (_server.hasArg("value")) {
        val = _server.arg("value").toInt();
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            val = doc["value"] | -1;
        }
    }
    if (val < 1 || val > 12) {
        sendError("Invalid 'value' (1-12)");
        return;
    }
    Commands::setTailLength(_state, (uint8_t)val);
    sendOk();
}

void HttpApi::handleSetStrobe() {
    int val = -1;
    if (_server.hasArg("value")) {
        val = _server.arg("value").toInt();
    } else if (_server.hasArg("plain")) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            val = doc["value"] | -1;
        }
    }
    if (val < 10) {
        sendError("Invalid 'value' (>=10)");
        return;
    }
    Commands::setStrobePeriod(_state, (uint16_t)val);
    sendOk();
}

void HttpApi::sendOk() {
    _server.send(200, "application/json", "{\"ok\":true}");
}

void HttpApi::sendError(const String& msg) {
    String out = "{\"ok\":false,\"error\":\"" + msg + "\"}";
    _server.send(400, "application/json", out);
}

uint32_t HttpApi::parseColor(const String& str) {
    String s = str;
    if (s.startsWith("#")) s = s.substring(1);
    return (uint32_t)strtoul(s.c_str(), nullptr, 16);
}
