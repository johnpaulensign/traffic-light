#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct AuthTokens {
    String accessToken;
    String refreshToken;
    unsigned long expiresAt;  // millis() timestamp when access token expires
    bool valid;
};

struct DeviceCodeResponse {
    String deviceCode;
    String userCode;
    String verificationUri;
    int expiresIn;
    int interval;
    bool valid;
};

class MicrosoftAuth {
public:
    MicrosoftAuth(const char* clientId, const char* tenantId);
    
    bool begin();
    
    bool hasValidToken();
    
    String getAccessToken();
    
    bool startDeviceFlow();
    
    bool pollForToken();
    
    bool refreshAccessToken();
    
    void clearTokens();
    
    const DeviceCodeResponse& getDeviceCodeResponse() const { return _deviceCode; }
    
private:
    const char* _clientId;
    const char* _tenantId;
    
    Preferences _prefs;
    AuthTokens _tokens;
    DeviceCodeResponse _deviceCode;
    unsigned long _lastPollTime;
    
    void loadTokens();
    void saveTokens();
    
    String buildTokenEndpoint();
    String buildDeviceCodeEndpoint();
};
