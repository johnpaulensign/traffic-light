#include "MicrosoftAuth.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* PREFS_NAMESPACE = "msauth";
static const char* KEY_ACCESS_TOKEN = "access";
static const char* KEY_REFRESH_TOKEN = "refresh";
static const char* KEY_EXPIRES_AT = "expires";

static const char* SCOPE = "Presence.Read offline_access";

MicrosoftAuth::MicrosoftAuth(const char* clientId, const char* tenantId)
    : _clientId(clientId)
    , _tenantId(tenantId)
    , _lastPollTime(0)
{
    _tokens = {String(), String(), 0, false};
    _deviceCode = {String(), String(), String(), 0, 0, false};
}

bool MicrosoftAuth::begin() {
    _prefs.begin(PREFS_NAMESPACE, false);
    loadTokens();
    return true;
}

void MicrosoftAuth::loadTokens() {
    _tokens.accessToken = _prefs.getString(KEY_ACCESS_TOKEN, "");
    _tokens.refreshToken = _prefs.getString(KEY_REFRESH_TOKEN, "");
    _tokens.expiresAt = _prefs.getULong(KEY_EXPIRES_AT, 0);
    _tokens.valid = _tokens.refreshToken.length() > 0;
    
    Serial.printf("[Auth] Loaded tokens - refresh token present: %s\n", 
                  _tokens.refreshToken.length() > 0 ? "yes" : "no");
}

void MicrosoftAuth::saveTokens() {
    _prefs.putString(KEY_ACCESS_TOKEN, _tokens.accessToken);
    _prefs.putString(KEY_REFRESH_TOKEN, _tokens.refreshToken);
    _prefs.putULong(KEY_EXPIRES_AT, _tokens.expiresAt);
    Serial.println("[Auth] Tokens saved to flash");
}

void MicrosoftAuth::clearTokens() {
    _tokens = {String(), String(), 0, false};
    _prefs.clear();
    Serial.println("[Auth] Tokens cleared");
}

String MicrosoftAuth::buildTokenEndpoint() {
    return String("https://login.microsoftonline.com/") + _tenantId + "/oauth2/v2.0/token";
}

String MicrosoftAuth::buildDeviceCodeEndpoint() {
    return String("https://login.microsoftonline.com/") + _tenantId + "/oauth2/v2.0/devicecode";
}

bool MicrosoftAuth::hasValidToken() {
    if (_tokens.accessToken.length() == 0) {
        return false;
    }
    // Check if token expires in the next 5 minutes
    unsigned long now = millis();
    if (_tokens.expiresAt > 0 && now > _tokens.expiresAt - 300000) {
        return false;
    }
    return true;
}

String MicrosoftAuth::getAccessToken() {
    if (hasValidToken()) {
        return _tokens.accessToken;
    }
    
    // Try to refresh if we have a refresh token
    if (_tokens.refreshToken.length() > 0) {
        Serial.println("[Auth] Access token expired, attempting refresh...");
        if (refreshAccessToken()) {
            return _tokens.accessToken;
        }
    }
    
    return String();
}

bool MicrosoftAuth::startDeviceFlow() {
    Serial.println("[Auth] Starting device code flow...");
    
    WiFiClientSecure client;
    client.setInsecure();  // TODO: Add proper CA cert for production
    
    HTTPClient http;
    http.begin(client, buildDeviceCodeEndpoint());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String body = "client_id=" + String(_clientId) + "&scope=" + String(SCOPE);
    
    int httpCode = http.POST(body);
    
    if (httpCode != 200) {
        Serial.printf("[Auth] Device code request failed: %d\n", httpCode);
        Serial.println(http.getString());
        http.end();
        return false;
    }
    
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.printf("[Auth] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    _deviceCode.deviceCode = doc["device_code"].as<String>();
    _deviceCode.userCode = doc["user_code"].as<String>();
    _deviceCode.verificationUri = doc["verification_uri"].as<String>();
    _deviceCode.expiresIn = doc["expires_in"].as<int>();
    _deviceCode.interval = doc["interval"].as<int>();
    _deviceCode.valid = true;
    _lastPollTime = 0;
    
    Serial.println("\n========================================");
    Serial.println("  MICROSOFT AUTHENTICATION REQUIRED");
    Serial.println("========================================");
    Serial.printf("  Go to: %s\n", _deviceCode.verificationUri.c_str());
    Serial.printf("  Enter code: %s\n", _deviceCode.userCode.c_str());
    Serial.println("========================================\n");
    
    return true;
}

bool MicrosoftAuth::pollForToken() {
    if (!_deviceCode.valid) {
        return false;
    }
    
    // Respect polling interval
    unsigned long now = millis();
    if (_lastPollTime > 0 && (now - _lastPollTime) < (_deviceCode.interval * 1000)) {
        return false;
    }
    _lastPollTime = now;
    
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.begin(client, buildTokenEndpoint());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String body = "grant_type=urn:ietf:params:oauth:grant-type:device_code";
    body += "&client_id=" + String(_clientId);
    body += "&device_code=" + _deviceCode.deviceCode;
    
    int httpCode = http.POST(body);
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.printf("[Auth] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    if (doc.containsKey("error")) {
        String errorCode = doc["error"].as<String>();
        if (errorCode == "authorization_pending") {
            // User hasn't completed auth yet, keep polling
            return false;
        } else if (errorCode == "slow_down") {
            _deviceCode.interval += 5;
            return false;
        } else {
            Serial.printf("[Auth] Token error: %s\n", errorCode.c_str());
            _deviceCode.valid = false;
            return false;
        }
    }
    
    // Success!
    _tokens.accessToken = doc["access_token"].as<String>();
    _tokens.refreshToken = doc["refresh_token"].as<String>();
    int expiresIn = doc["expires_in"].as<int>();
    _tokens.expiresAt = millis() + (expiresIn * 1000UL);
    _tokens.valid = true;
    _deviceCode.valid = false;
    
    saveTokens();
    Serial.println("[Auth] Authentication successful!");
    
    return true;
}

bool MicrosoftAuth::refreshAccessToken() {
    if (_tokens.refreshToken.length() == 0) {
        return false;
    }
    
    Serial.println("[Auth] Refreshing access token...");
    
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.begin(client, buildTokenEndpoint());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String body = "grant_type=refresh_token";
    body += "&client_id=" + String(_clientId);
    body += "&refresh_token=" + _tokens.refreshToken;
    body += "&scope=" + String(SCOPE);
    
    int httpCode = http.POST(body);
    String response = http.getString();
    http.end();
    
    if (httpCode != 200) {
        Serial.printf("[Auth] Refresh failed: %d\n", httpCode);
        Serial.println(response);
        // Clear tokens if refresh fails - will need to re-auth
        clearTokens();
        return false;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.printf("[Auth] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    _tokens.accessToken = doc["access_token"].as<String>();
    if (doc.containsKey("refresh_token")) {
        _tokens.refreshToken = doc["refresh_token"].as<String>();
    }
    int expiresIn = doc["expires_in"].as<int>();
    _tokens.expiresAt = millis() + (expiresIn * 1000UL);
    _tokens.valid = true;
    
    saveTokens();
    Serial.println("[Auth] Token refreshed successfully");
    
    return true;
}
