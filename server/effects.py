"""Effect definitions and presence-to-effect mapping."""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Optional


class EffectType(Enum):
    """Types of LED effects."""
    SOLID = auto()
    FADE = auto()
    STROBE_THEN_SOLID = auto()
    OFF = auto()


@dataclass
class Effect:
    """Describes an LED effect with color and type."""
    effect_type: EffectType
    color: str  # Hex color like "#FF0000"
    strobe_duration_seconds: float = 1.0  # For STROBE_THEN_SOLID


# Presence to effect mapping
PRESENCE_EFFECTS = {
    # Green (solid)
    'Available': Effect(EffectType.SOLID, "#00FF00"),
    
    # Yellow (fade)
    'Away': Effect(EffectType.FADE, "#FF9600"),
    'BeRightBack': Effect(EffectType.FADE, "#FF9600"),
    
    # Red (strobe then solid)
    'Busy': Effect(EffectType.STROBE_THEN_SOLID, "#FF0000"),
    'InAMeeting': Effect(EffectType.STROBE_THEN_SOLID, "#FF0000"),
    'DoNotDisturb': Effect(EffectType.STROBE_THEN_SOLID, "#FF0000"),
    'InACall': Effect(EffectType.STROBE_THEN_SOLID, "#FF0000"),
    'Presenting': Effect(EffectType.STROBE_THEN_SOLID, "#FF0000"),
    
    # Off
    'Offline': Effect(EffectType.OFF, "#000000"),
    'PresenceUnknown': Effect(EffectType.OFF, "#000000"),
}

# Default effect for unknown presence states
DEFAULT_EFFECT = Effect(EffectType.SOLID, "#0000FF")  # Blue for unknown


def get_effect_for_presence(presence: str) -> Effect:
    """
    Get the appropriate effect for a Teams presence status.
    
    Args:
        presence: Teams availability string (e.g., "Available", "Busy")
    
    Returns:
        Effect to apply
    """
    return PRESENCE_EFFECTS.get(presence, DEFAULT_EFFECT)
