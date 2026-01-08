#include "TeamsPresence.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* GRAPH_PRESENCE_ENDPOINT = "https://graph.microsoft.com/v1.0/me/presence";

TeamsPresence::TeamsPresence(MicrosoftAuth& auth)
    : _auth(auth)
    , _presence(Presence::Unknown)
{
}

bool TeamsPresence::fetchPresence() {
    String token = _auth.getAccessToken();
    if (token.length() == 0) {
        Serial.println("[Presence] No valid access token");
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure();  // TODO: Add proper CA cert for production
    
    HTTPClient http;
    http.begin(client, GRAPH_PRESENCE_ENDPOINT);
    http.addHeader("Authorization", "Bearer " + token);
    
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        Serial.printf("[Presence] Request failed: %d\n", httpCode);
        if (httpCode == 401) {
            Serial.println("[Presence] Token may be expired");
        }
        http.end();
        return false;
    }
    
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.printf("[Presence] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    String availability = doc["availability"].as<String>();
    _presence = parsePresence(availability);
    
    Serial.printf("[Presence] Current status: %s\n", availability.c_str());
    
    return true;
}

Presence TeamsPresence::parsePresence(const String& availability) {
    if (availability == "Available") return Presence::Available;
    if (availability == "Away") return Presence::Away;
    if (availability == "BeRightBack") return Presence::BeRightBack;
    if (availability == "Busy") return Presence::Busy;
    if (availability == "DoNotDisturb") return Presence::DoNotDisturb;
    if (availability == "InACall") return Presence::InACall;
    if (availability == "InAMeeting") return Presence::InAMeeting;
    if (availability == "Presenting") return Presence::Presenting;
    if (availability == "Offline") return Presence::Offline;
    return Presence::Unknown;
}

const char* TeamsPresence::getPresenceString() const {
    switch (_presence) {
        case Presence::Available: return "Available";
        case Presence::Away: return "Away";
        case Presence::BeRightBack: return "BeRightBack";
        case Presence::Busy: return "Busy";
        case Presence::DoNotDisturb: return "DoNotDisturb";
        case Presence::InACall: return "InACall";
        case Presence::InAMeeting: return "InAMeeting";
        case Presence::Presenting: return "Presenting";
        case Presence::Offline: return "Offline";
        default: return "Unknown";
    }
}

PresenceEffect TeamsPresence::getEffect() const {
    return mapPresenceToEffect(_presence);
}

PresenceEffect TeamsPresence::mapPresenceToEffect(Presence presence) {
    switch (presence) {
        case Presence::Available:
            return {EffectType::Solid, 0x00FF00};  // Green
            
        case Presence::Away:
        case Presence::BeRightBack:
            return {EffectType::Fade, 0xFF9600};  // Orange/Yellow
            
        case Presence::Busy:
        case Presence::DoNotDisturb:
        case Presence::InACall:
        case Presence::InAMeeting:
        case Presence::Presenting:
            return {EffectType::StrobeThenSolid, 0xFF0000};  // Red
            
        case Presence::Offline:
            return {EffectType::Off, 0x000000};
            
        default:
            return {EffectType::Solid, 0x0000FF};  // Blue for unknown
    }
}
