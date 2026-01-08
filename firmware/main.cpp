#include <Arduino.h>
#include <WiFi.h>

#include "Config.h"
#include "AppState.h"
#include "LedRing.h"
#include "AnimationManager.h"
#include "ButtonInput.h"
#include "Commands.h"
#include "HttpApi.h"
#include "MicrosoftAuth.h"
#include "TeamsPresence.h"

#include "animations/FadeAnimation.h"
#include "animations/SpinAnimation.h"
#include "animations/SpinTailAnimation.h"
#include "animations/StrobeAnimation.h"
#include "animations/SolidAnimation.h"
#include "animations/PixelsAnimation.h"

// ============ WiFi Configuration ============
// TODO: Replace with your WiFi credentials
const char* WIFI_SSID = "Loading...";
const char* WIFI_PASS = "wasthatyourstomach";

// ============ Global Objects ============
AppState appState;
LedRing ledRing(Config::LED_PIN, Config::NUM_PIXELS);
AnimationManager animMgr;
ButtonInput button(Config::BUTTON_PIN, true);  // active-low (pull-up)
HttpApi* httpApi = nullptr;

// Microsoft Graph / Teams presence
MicrosoftAuth msAuth(Config::MS_CLIENT_ID, Config::MS_TENANT_ID);
TeamsPresence teamsPresence(msAuth);

// Presence polling state
unsigned long lastPresencePoll = 0;
bool authInProgress = false;
Presence lastPresence = Presence::Unknown;
unsigned long strobeStartTime = 0;
bool inStrobePhase = false;
String strobeThen = "";

// Animation instances
FadeAnimation fadeAnim;
SpinAnimation spinAnim;
SpinTailAnimation spinTailAnim;
StrobeAnimation strobeAnim;
SolidAnimation solidAnim;
PixelsAnimation pixelsAnim;

void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" Failed to connect. HTTP API will not be available.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Teams Ring Starting ===");

    // Initialize LED ring
    ledRing.begin();
    ledRing.setBrightness(appState.brightness);
    ledRing.clear();
    ledRing.show();
    Serial.println("LED ring initialized");

    // Register animations
    animMgr.addAnimation(&fadeAnim);
    animMgr.addAnimation(&spinAnim);
    animMgr.addAnimation(&spinTailAnim);
    animMgr.addAnimation(&strobeAnim);
    animMgr.addAnimation(&solidAnim);
    animMgr.addAnimation(&pixelsAnim);
    animMgr.setActive(appState.currentAnimationName, appState);
    Serial.println("Animations registered");

    // Initialize button
    button.begin();
    Serial.println("Button initialized");

    // Connect to WiFi and start HTTP API
    connectWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        httpApi = new HttpApi(appState, animMgr, ledRing);
        httpApi->begin();
        Serial.println("HTTP API started on port 80");
        
        // Initialize Microsoft auth
        msAuth.begin();
        
        // Check if we have a valid token, otherwise start device flow
        if (!msAuth.hasValidToken()) {
            Serial.println("No valid token found, starting device flow...");
            if (msAuth.startDeviceFlow()) {
                authInProgress = true;
            }
        } else {
            Serial.println("Valid token found, will poll presence");
        }
    }

    Serial.println("=== Setup Complete ===\n");
}

void loop() {
    uint32_t nowMs = millis();

    // Handle button input
    ButtonEvent evt = button.update(nowMs);
    switch (evt) {
        case ButtonEvent::Click1:
            Serial.println("Button: Single click -> Next animation");
            Commands::nextAnimation(appState, animMgr);
            break;
        case ButtonEvent::Click2:
            Serial.println("Button: Double click -> Toggle strobe");
            if (String(animMgr.currentName()) == "strobe") {
                Commands::setAnimation(appState, animMgr, "fade");
            } else {
                Commands::setAnimation(appState, animMgr, "strobe");
            }
            break;
        case ButtonEvent::Click3:
            Serial.println("Button: Triple click -> Cycle color");
            {
                // Cycle through some preset colors
                static uint8_t colorIdx = 0;
                const uint32_t colors[] = {0x0000FF, 0x00FF00, 0xFF0000, 0xFF00FF, 0x00FFFF, 0xFFFF00, 0xFFFFFF};
                colorIdx = (colorIdx + 1) % 7;
                Commands::setColor(appState, colors[colorIdx]);
            }
            break;
        case ButtonEvent::Hold:
            Serial.println("Button: Hold -> Toggle power");
            Commands::togglePower(appState, ledRing);
            break;
        default:
            break;
    }

    // Handle HTTP requests
    if (httpApi) {
        httpApi->poll();
    }

    // Handle Microsoft auth device flow polling
    if (authInProgress) {
        if (msAuth.pollForToken()) {
            authInProgress = false;
            Serial.println("Authentication complete! Starting presence polling.");
            // Immediately poll presence after auth
            lastPresencePoll = 0;
        }
    }
    
    // Poll Teams presence at configured interval
    if (!authInProgress && WiFi.status() == WL_CONNECTED) {
        if (nowMs - lastPresencePoll >= Config::PRESENCE_POLL_INTERVAL_MS || lastPresencePoll == 0) {
            lastPresencePoll = nowMs;
            
            if (teamsPresence.fetchPresence()) {
                Presence currentPresence = teamsPresence.getPresence();
                
                // Only update if presence changed
                if (currentPresence != lastPresence) {
                    Serial.printf("Presence changed: %s -> %s\n", 
                                  lastPresence == Presence::Unknown ? "Unknown" : teamsPresence.getPresenceString(),
                                  teamsPresence.getPresenceString());
                    lastPresence = currentPresence;
                    
                    PresenceEffect effect = teamsPresence.getEffect();
                    
                    // Apply the effect
                    appState.primaryColor = effect.color;
                    
                    // Clear all pixels first
                    for (int i = 0; i < Config::NUM_PIXELS; i++) {
                        appState.pixelColors[i] = 0x000000;
                    }

                    // Light only the relevant LED(s)
                    switch (effect.trafficLight) {
                        case TrafficLightState::Bottom:
                            appState.pixelColors[0] = effect.color;
                            effect.type = EffectType::StrobeThenPixel;
                            break;
                        case TrafficLightState::Middle:
                            appState.pixelColors[1] = effect.color;
                            effect.type = EffectType::Pixel;
                            break;
                        case TrafficLightState::Top:
                            appState.pixelColors[2] = effect.color;
                            effect.type = EffectType::Pixel;
                            break;
                        case TrafficLightState::All:
                            for (int i = 0; i < Config::NUM_PIXELS; i++) {
                                appState.pixelColors[i] = effect.color;
                            }
                            break;
                    }

                    switch (effect.type) {
                        case EffectType::Solid:
                            Commands::setAnimation(appState, animMgr, "solid");
                            break;
                        case EffectType::Pixel:
                            Commands::setAnimation(appState, animMgr, "pixels");
                            break;
                        case EffectType::StrobeThenPixel:
                            Serial.println("Starting strobe -> pixels");
                            Commands::setAnimation(appState, animMgr, "strobe");
                            strobeStartTime = nowMs;
                            inStrobePhase = true;
                            strobeThen = "pixels";
                            break;
                        case EffectType::Fade:
                            Commands::setAnimation(appState, animMgr, "fade");
                            break;
                        case EffectType::StrobeThenSolid:
                            Serial.println("Starting strobe -> solid");
                            Commands::setAnimation(appState, animMgr, "strobe");
                            strobeStartTime = nowMs;
                            inStrobePhase = true;
                            strobeThen = "solid";
                            break;
                        case EffectType::Off:
                            appState.powerOn = false;
                            ledRing.clear();
                            ledRing.show();
                            break;
                    }

                    // Ensure power is on for non-off effects
                    if (effect.type != EffectType::Off) {
                        appState.powerOn = true;
                    }
                }
            } else if (!msAuth.hasValidToken()) {
                // Token expired or invalid, restart auth flow
                Serial.println("Token invalid, restarting device flow...");
                if (msAuth.startDeviceFlow()) {
                    authInProgress = true;
                }
            }
        }
    }
    
    // Handle strobe -> solid transition
    if (inStrobePhase && (nowMs - strobeStartTime >= Config::STROBE_DURATION_MS)) {
        Commands::setAnimation(appState, animMgr, strobeThen);
        inStrobePhase = false;
    }

    // Update animation (only if powered on)
    if (appState.powerOn) {
        animMgr.update(nowMs, appState, ledRing);
    }
}
