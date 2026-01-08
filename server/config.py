"""Configuration loader for the Teams Ring server."""

import json
import os
from dataclasses import dataclass
from typing import Optional

@dataclass
class Config:
    client_id: str
    tenant_id: str
    esp32_host: str
    refresh_interval_seconds: int = 60
    strobe_duration_seconds: float = 1.0
    strobe_period_ms: int = 100
    fade_speed_ms: int = 40
    mode: str = "trafficlight"
    brightness: int = 128

def load_config(path: Optional[str] = None) -> Config:
    """Load configuration from settings.json."""
    if path is None:
        path = os.path.join(os.path.dirname(__file__), 'settings.json')
    
    with open(path, 'r') as f:
        data = json.load(f)
    
    return Config(
        client_id=data['client_id'],
        tenant_id=data['tenant_id'],
        esp32_host=data['esp32_host'],
        refresh_interval_seconds=data.get('refresh_interval_seconds', 60),
        strobe_duration_seconds=data.get('strobe_duration_seconds', 1.0),
        strobe_period_ms=data.get('strobe_period_ms', 100),
        fade_speed_ms=data.get('fade_speed_ms', 40),
        mode=data.get('mode', 'trafficlight'),
        brightness=data.get('brightness', 128),
    )
