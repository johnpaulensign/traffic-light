"""HTTP client for ESP32 LED ring control."""

from typing import Any, Dict, Optional
import requests


class Esp32Client:
    """Typed wrapper around ESP32 HTTP API endpoints."""

    def __init__(self, host: str, timeout: float = 5.0):
        """
        Initialize the ESP32 client.
        
        Args:
            host: Base URL of the ESP32 (e.g., "http://192.168.1.50")
            timeout: Request timeout in seconds
        """
        self.host = host.rstrip('/')
        self.timeout = timeout
        self.session = requests.Session()

    def _post(self, endpoint: str, data: Dict[str, Any]) -> Dict[str, Any]:
        """POST JSON to an endpoint and return the response."""
        url = f"{self.host}{endpoint}"
        resp = self.session.post(url, json=data, timeout=self.timeout)
        resp.raise_for_status()
        return resp.json()

    def _get(self, endpoint: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """GET from an endpoint and return the response."""
        url = f"{self.host}{endpoint}"
        resp = self.session.get(url, params=params, timeout=self.timeout)
        resp.raise_for_status()
        return resp.json()

    def get_status(self) -> Dict[str, Any]:
        """Get current device status."""
        return self._get("/status")

    def get_animations(self) -> list:
        """Get list of available animation names."""
        return self._get("/animations")

    def set_power(self, on: bool) -> None:
        """Turn the LED ring on or off."""
        self._post("/power", {"on": on})

    def set_brightness(self, value: int) -> None:
        """Set brightness (0-255)."""
        self._post("/brightness", {"value": value})

    def set_color(self, rgb_hex: str) -> None:
        """
        Set the primary color.
        
        Args:
            rgb_hex: Color in hex format (e.g., "#FF0000" for red)
        """
        self._post("/color", {"rgb": rgb_hex})

    def set_animation(self, name: str) -> None:
        """
        Set the active animation.
        
        Args:
            name: Animation name (fade, spin, spinTail, strobe, solid)
        """
        self._post("/animation", {"name": name})

    def set_speed(self, ms: int) -> None:
        """Set animation step interval in milliseconds."""
        self._post("/speed", {"value": ms})

    def set_strobe_period(self, ms: int) -> None:
        """Set strobe on+off cycle period in milliseconds."""
        self._post("/strobe", {"value": ms})

    def set_tail_length(self, length: int) -> None:
        """Set tail length for spinTail animation (1-12)."""
        self._post("/tail", {"value": length})

    def set_pixel(self, position: int, rgb_hex: str) -> None:
        """
        Set a single pixel color.
        
        Args:
            position: Pixel index (0-based)
            rgb_hex: Color in hex format (e.g., "#FF0000")
        """
        self._post("/pixel", {"position": position, "rgb": rgb_hex})

    def set_pixels(self, pixels: list) -> None:
        """
        Set multiple pixel colors at once.
        
        Args:
            pixels: List of (position, rgb_hex) tuples or dicts with position/rgb keys
        """
        # Convert tuples to dicts if needed
        pixel_data = []
        for p in pixels:
            if isinstance(p, tuple):
                pixel_data.append({"position": p[0], "rgb": p[1]})
            else:
                pixel_data.append(p)
        
        self._post("/pixels", pixel_data)
