#!/usr/bin/env python3
"""
Director: Main control loop that polls Teams presence and drives the ESP32 LED ring.

Handles the state machine for effects, including timed transitions
(e.g., strobe for 1 second then switch to solid).
"""

import time
import logging
from typing import Optional
from dataclasses import dataclass

from .config import Config, load_config
from .teams_client import TeamsClient
from .esp32_client import Esp32Client
from .effects import Effect, EffectType, get_effect_for_presence
from .display_modes import DisplayMode, create_display_mode

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@dataclass
class PendingTransition:
    """A scheduled transition to a different animation."""
    trigger_time: float  # monotonic time
    animation: str
    color: str


class Director:
    """
    State machine that:
    - Polls Teams presence at regular intervals
    - Applies effects to the ESP32 via the configured display mode
    - Handles timed transitions (strobe -> solid) for ring mode
    """

    def __init__(self, config: Config):
        self.config = config
        self.teams = TeamsClient(config.client_id, config.tenant_id)
        self.esp32 = Esp32Client(config.esp32_host, timeout=3.0)
        self.display_mode: DisplayMode = create_display_mode(config.mode)
        
        self.current_presence: Optional[str] = None
        self.current_effect: Optional[Effect] = None
        self.pending_transition: Optional[PendingTransition] = None
        self.last_poll_time: float = 0

    def apply_effect(self, effect: Effect) -> None:
        """Apply an effect to the ESP32 if it doesn't already match."""
        # Check if ESP32 already has the desired state
        if self.display_mode.matches_effect(self.esp32, effect, self.pending_transition):
            logger.debug(f"ESP32 already matches effect {effect.effect_type.name}, skipping")
            return
        
        logger.info(f"Applying effect: {effect.effect_type.name} with color {effect.color}")
        
        try:
            # Clear any pending transition when applying a new effect
            self.pending_transition = None
            
            # Apply the effect via the display mode
            self.display_mode.apply_effect(self.esp32, effect, self.config)
            
            # Schedule transition for strobe effects (only relevant for ring mode)
            if effect.effect_type == EffectType.STROBE_THEN_SOLID:
                transition_time = time.monotonic() + self.config.strobe_duration_seconds
                self.pending_transition = PendingTransition(
                    trigger_time=transition_time,
                    animation="solid",
                    color=effect.color
                )
                logger.info(f"Scheduled transition to solid in {self.config.strobe_duration_seconds}s")
                
        except Exception as e:
            logger.warning(f"Failed to apply effect: {e}")

    def check_pending_transition(self) -> None:
        """Check and execute any pending timed transitions."""
        if self.pending_transition is None:
            return
            
        now = time.monotonic()
        if now >= self.pending_transition.trigger_time:
            logger.info(f"Executing pending transition to {self.pending_transition.animation}")
            try:
                self.display_mode.apply_transition(self.esp32, self.pending_transition.animation)
            except Exception as e:
                logger.warning(f"Failed to execute transition: {e}")
            self.pending_transition = None

    def poll_presence(self) -> None:
        """Poll Teams presence and apply effect every time."""
        try:
            presence = self.teams.get_presence()
            logger.info(f"Teams presence: {presence}")
            
            if presence != self.current_presence:
                logger.info(f"Presence changed: {self.current_presence} -> {presence}")
            
            self.current_presence = presence
            effect = get_effect_for_presence(presence)
            self.current_effect = effect
            self.apply_effect(effect)
                
        except Exception as e:
            logger.error(f"Failed to poll presence: {e}")

    def run(self) -> None:
        """Main run loop."""
        logger.info("Director starting...")
        logger.info(f"Display mode: {self.config.mode}")
        logger.info(f"ESP32 host: {self.config.esp32_host}")
        logger.info(f"Refresh interval: {self.config.refresh_interval_seconds}s")
        
        # Set initial brightness
        try:
            logger.info(f"Setting brightness to {self.config.brightness}")
            self.esp32.set_brightness(self.config.brightness)
        except Exception as e:
            logger.warning(f"Failed to set initial brightness: {e}")
        
        # Initial poll
        self.poll_presence()
        self.last_poll_time = time.monotonic()
        
        while True:
            now = time.monotonic()
            
            # Check for pending transitions (high frequency)
            self.check_pending_transition()
            
            # Poll presence at configured interval
            if now - self.last_poll_time >= self.config.refresh_interval_seconds:
                self.poll_presence()
                self.last_poll_time = now
            
            # Small sleep to avoid busy-waiting
            time.sleep(0.05)


def main():
    """Entry point."""
    config = load_config()
    director = Director(config)
    
    try:
        director.run()
    except KeyboardInterrupt:
        logger.info("Shutting down...")


if __name__ == "__main__":
    main()
