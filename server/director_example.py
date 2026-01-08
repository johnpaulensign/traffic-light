#!/usr/bin/env python3

import json
import requests
import time
import os
from msal import PublicClientApplication

REFRESH_INTERVAL = 5
SETTINGS_PATH = os.path.join(os.path.dirname(__file__), 'devsettings.json')
TOKEN_CACHE_PATH = os.path.join(os.path.dirname(__file__), 'token_cache.bin')

with open(SETTINGS_PATH) as f:
    settings = json.load(f)

CLIENT_ID = settings['client_id']
TENANT_ID = settings['tenant_id']
ESP32_HOST = settings['esp32_host']

# Microsoft Graph endpoints
AUTHORITY = f"https://login.microsoftonline.com/{TENANT_ID}"
SCOPE = ["Presence.Read"]
PRESENCE_ENDPOINT = "https://graph.microsoft.com/v1.0/me/presence"

# LED mapping
STATUS_TO_LED = {
    'Available': {'position': 0, 'r': 0, 'g': 255, 'b': 0},      # Green
    'Away': {'position': 1, 'r': 255, 'g': 150, 'b': 0},         # Yellow
    'BeRightBack': {'position': 1, 'r': 255, 'g': 150, 'b': 0},   # Yellow
    'Busy': {'position': 2, 'r': 255, 'g': 0, 'b': 0},           # Red
    'InAMeeting': {'position': 2, 'r': 255, 'g': 0, 'b': 0},     # Red
    'DoNotDisturb': {'position': 2, 'r': 255, 'g': 0, 'b': 0},   # Red
    'Offline': {'position': 0, 'r': 0, 'g': 0, 'b': 0},         # Off
}

# Use a persistent token cache
def get_token_cache():
    from msal import SerializableTokenCache
    cache = SerializableTokenCache()
    if os.path.exists(TOKEN_CACHE_PATH):
        with open(TOKEN_CACHE_PATH, 'rb') as f:
            cache.deserialize(f.read().decode())
    return cache

def save_token_cache(cache):
    if cache.has_state_changed:
        with open(TOKEN_CACHE_PATH, 'wb') as f:
            f.write(cache.serialize().encode())

# Authenticate with MS Graph using device code flow
def get_access_token():
    cache = get_token_cache()
    app = PublicClientApplication(CLIENT_ID, authority=AUTHORITY, token_cache=cache)
    accounts = app.get_accounts()
    if accounts:
        result = app.acquire_token_silent(SCOPE, account=accounts[0])
        if result and 'access_token' in result:
            save_token_cache(cache)
            return result['access_token']
    flow = app.initiate_device_flow(scopes=SCOPE)
    if 'user_code' not in flow:
        raise Exception(f"Failed to create device flow: {flow}")
    print(f"To authenticate, use a web browser to visit {flow['verification_uri']} and enter the code: {flow['user_code']}")
    result = app.acquire_token_by_device_flow(flow)
    if 'access_token' in result:
        save_token_cache(cache)
        return result['access_token']
    else:
        raise Exception(f"Could not obtain access token: {result}")

def get_teams_presence(token):
    headers = {'Authorization': f'Bearer {token}'}
    resp = requests.get(PRESENCE_ENDPOINT, headers=headers)
    resp.raise_for_status()
    return resp.json()['availability']

def set_leds(status):
    # Set all LEDs at once using /leds endpoint
    # Only one LED is on at a time, others are off
    led_array = [
        {"on": False, "r": 0, "g": 0, "b": 0},
        {"on": False, "r": 0, "g": 0, "b": 0},
        {"on": False, "r": 0, "g": 0, "b": 0}
    ]
    led = STATUS_TO_LED.get(status, STATUS_TO_LED['Available'])

    if(str.lower(status) == "offline"):
        print("Setting status to Offline")
    else:
        led_array[led['position']] = {
            "on": True,
            "r": led['r'],
            "g": led['g'],
            "b": led['b']
        }
    resp = requests.post(f"{ESP32_HOST}/leds", json=led_array)
    resp.raise_for_status()
    print(f"Set LEDs for status {status}: {resp.text}")

def main():
    previous_status = None
    while True:
        try:
            token = get_access_token()
            status = get_teams_presence(token)
            print(f"Teams status: {status}")
            if status != previous_status:
                set_leds(status)
                previous_status = status
        except Exception as e:
            print(f"Error: {e}")

        # Get the current time and sleep until the next multiple of REFRESH_INTERVAL seconds
        now = time.time()
        sleep_time = REFRESH_INTERVAL - (now % REFRESH_INTERVAL)
        time.sleep(sleep_time)

if __name__ == "__main__":
    main()
