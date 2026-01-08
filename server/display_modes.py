"""Display mode abstraction for different LED configurations."""

from abc import ABC, abstractmethod
from enum import Enum
from typing import List, Tuple, Optional
import logging

from .esp32_client import Esp32Client
from .effects import Effect, EffectType

logger = logging.getLogger(__name__)


class DisplayMode(ABC):
    """Base class for display modes."""
    
    @abstractmethod
    def apply_effect(self, esp32: Esp32Client, effect: Effect, config) -> None:
        """Apply an effect to the ESP32."""
        pass
    
    @abstractmethod
    def matches_effect(self, esp32: Esp32Client, effect: Effect, pending_transition) -> bool:
        """Check if ESP32 current state matches the desired effect."""
        pass
    
    @abstractmethod
    def apply_transition(self, esp32: Esp32Client, animation: str) -> None:
        """Apply a pending transition (e.g., strobe -> solid)."""
        pass


class RingMode(DisplayMode):
    """
    Ring mode: Uses the full LED ring with animations.
    - Solid: all LEDs same color
    - Fade: breathing effect
    - Strobe: flashing effect
    """
    
    def _get_expected_animation(self, effect: Effect, pending_transition) -> Optional[str]:
        """Get the expected animation name for an effect."""
        if effect.effect_type == EffectType.OFF:
            return None
        elif effect.effect_type == EffectType.SOLID:
            return "solid"
        elif effect.effect_type == EffectType.FADE:
            return "fade"
        elif effect.effect_type == EffectType.STROBE_THEN_SOLID:
            if pending_transition is not None:
                return "strobe"
            return "solid"
        return None
    
    def matches_effect(self, esp32: Esp32Client, effect: Effect, pending_transition) -> bool:
        """Check if ESP32 current state matches the desired effect."""
        try:
            status = esp32.get_status()
        except Exception as e:
            logger.warning(f"Failed to get ESP32 status: {e}")
            return False
        
        if effect.effect_type == EffectType.OFF:
            return not status.get("powerOn", True)
        
        if not status.get("powerOn", False):
            return False
        
        esp_color = status.get("color", "").upper()
        expected_color = effect.color.upper()
        if esp_color != expected_color:
            return False
        
        esp_animation = status.get("animation", "")
        expected_animation = self._get_expected_animation(effect, pending_transition)
        if expected_animation and esp_animation != expected_animation:
            return False
        
        return True
    
    def apply_effect(self, esp32: Esp32Client, effect: Effect, config) -> None:
        """Apply an effect using ring animations."""
        if effect.effect_type == EffectType.OFF:
            esp32.set_power(False)
            
        elif effect.effect_type == EffectType.SOLID:
            esp32.set_power(True)
            esp32.set_color(effect.color)
            esp32.set_animation("solid")
            
        elif effect.effect_type == EffectType.FADE:
            esp32.set_power(True)
            esp32.set_color(effect.color)
            esp32.set_speed(config.fade_speed_ms)
            esp32.set_animation("fade")
            
        elif effect.effect_type == EffectType.STROBE_THEN_SOLID:
            esp32.set_power(True)
            esp32.set_color(effect.color)
            esp32.set_strobe_period(config.strobe_period_ms)
            esp32.set_animation("strobe")
    
    def apply_transition(self, esp32: Esp32Client, animation: str) -> None:
        """Apply a pending transition."""
        esp32.set_animation(animation)


class TrafficLightMode(DisplayMode):
    """
    Traffic light mode: Uses 3 LEDs as a traffic light.
    - Position 0 (bottom): Green
    - Position 1 (middle): Yellow
    - Position 2 (top): Red
    
    Only one LED is lit at a time based on presence status.
    """
    
    # LED positions
    GREEN_POS = 0
    YELLOW_POS = 1
    RED_POS = 2
    
    # Colors
    GREEN = "#00FF00"
    YELLOW = "#FF9600"
    RED = "#FF0000"
    OFF = "#000000"
    
    def __init__(self):
        # Track last applied effect type since /status doesn't expose per-pixel state
        self._last_effect_type: Optional[EffectType] = None
    
    def _get_expected_animation(self, effect: Effect, pending_transition) -> Optional[str]:
        """Get the expected animation name for an effect."""
        if effect.effect_type == EffectType.OFF:
            return None
        elif effect.effect_type == EffectType.STROBE_THEN_SOLID:
            # During strobe phase (pending transition exists), expect strobe
            # After transition completes (no pending), expect pixels
            if pending_transition is not None:
                return "strobe"
            return "pixels"
        # All other effects use pixels animation
        return "pixels"
    
    def _get_traffic_light_state(self, effect: Effect) -> List[Tuple[int, str]]:
        """
        Get the pixel states for a traffic light based on effect.
        Returns list of (position, color) tuples.
        """
        if effect.effect_type == EffectType.OFF:
            return [
                (self.GREEN_POS, self.OFF),
                (self.YELLOW_POS, self.OFF),
                (self.RED_POS, self.OFF),
            ]
        elif effect.effect_type == EffectType.SOLID:
            # Green = Available
            if effect.color.upper() == self.GREEN.upper():
                return [
                    (self.GREEN_POS, self.GREEN),
                    (self.YELLOW_POS, self.OFF),
                    (self.RED_POS, self.OFF),
                ]
            # Unknown solid color - show as blue on all
            return [
                (self.GREEN_POS, effect.color),
                (self.YELLOW_POS, self.OFF),
                (self.RED_POS, self.OFF),
            ]
        elif effect.effect_type == EffectType.FADE:
            # Yellow = Away/BeRightBack
            return [
                (self.GREEN_POS, self.OFF),
                (self.YELLOW_POS, self.YELLOW),
                (self.RED_POS, self.OFF),
            ]
        elif effect.effect_type == EffectType.STROBE_THEN_SOLID:
            # Red = Busy/DoNotDisturb/InAMeeting/etc.
            return [
                (self.GREEN_POS, self.OFF),
                (self.YELLOW_POS, self.OFF),
                (self.RED_POS, self.RED),
            ]
        
        # Default: all off
        return [
            (self.GREEN_POS, self.OFF),
            (self.YELLOW_POS, self.OFF),
            (self.RED_POS, self.OFF),
        ]
    
    def matches_effect(self, esp32: Esp32Client, effect: Effect, pending_transition) -> bool:
        """Check if ESP32 current state matches the desired effect."""
        try:
            status = esp32.get_status()
        except Exception as e:
            logger.warning(f"Failed to get ESP32 status: {e}")
            return False
        
        # Check power state for OFF effects
        if effect.effect_type == EffectType.OFF:
            return not status.get("powerOn", True)
        
        # ESP must be on
        if not status.get("powerOn", False):
            return False
        
        # Check animation matches expected
        esp_animation = status.get("animation", "")
        expected_animation = self._get_expected_animation(effect, pending_transition)
        if expected_animation and esp_animation != expected_animation:
            return False
        
        # For pixels animation, also check effect type matches what we last applied
        # since /status doesn't expose per-pixel colors
        if esp_animation == "pixels" and self._last_effect_type != effect.effect_type:
            return False
        
        return True
    
    def apply_effect(self, esp32: Esp32Client, effect: Effect, config) -> None:
        """Apply an effect using per-pixel control for traffic light."""
        # Always ensure power is on (unless OFF effect)
        if effect.effect_type == EffectType.OFF:
            esp32.set_power(False)
            self._last_effect_type = effect.effect_type
            return
        
        esp32.set_power(True)
        
        # For STROBE_THEN_SOLID (red/busy), use strobe animation first
        if effect.effect_type == EffectType.STROBE_THEN_SOLID:
            esp32.set_color(self.RED)
            esp32.set_strobe_period(config.strobe_period_ms)
            esp32.set_animation("strobe")
        else:
            # For other effects, set pixels directly
            pixels = self._get_traffic_light_state(effect)
            esp32.set_pixels(pixels)
        
        self._last_effect_type = effect.effect_type
    
    def apply_transition(self, esp32: Esp32Client, animation: str) -> None:
        """
        Apply a pending transition.
        For traffic light mode, switch from strobe to solid red pixel.
        """
        # After strobe, show solid red on the red LED
        pixels = [
            (self.GREEN_POS, self.OFF),
            (self.YELLOW_POS, self.OFF),
            (self.RED_POS, self.RED),
        ]
        esp32.set_pixels(pixels)


def create_display_mode(mode_name: str) -> DisplayMode:
    """
    Factory function to create a display mode by name.
    
    Args:
        mode_name: "ring" or "trafficlight"
    
    Returns:
        DisplayMode instance
    
    Raises:
        ValueError: If mode_name is not recognized
    """
    modes = {
        "ring": RingMode,
        "trafficlight": TrafficLightMode,
    }
    
    mode_class = modes.get(mode_name.lower())
    if mode_class is None:
        valid_modes = ", ".join(modes.keys())
        raise ValueError(f"Unknown display mode '{mode_name}'. Valid modes: {valid_modes}")
    
    return mode_class()
