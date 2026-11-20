MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.2 adds selected MIDI text metadata directly beneath the piece title in the main display: Marker (meta-event 0x06), Text Event (0x01) and Copyright (0x02). The events are shown in their original file order; other text meta-events remain available in the INFO window.

The piece title and its selected metadata lines are now positioned as a single block anchored to the bottom edge of the visual area, immediately above the progress strip. Increasing the window height therefore creates additional space above this block instead of pushing the title and metadata downward from a fixed top coordinate. The existing track-count and ticks-per-quarter information remains in its lower-left position.

The section-independent SETTINGS.INI lookup introduced in v0.1 is retained: each setting is found by key name anywhere in the file, regardless of its [section]. Keep setting keys unique. The renamed window geometry, velocity, progress and octave-guide settings remain in use.

All v0.1 functionality is retained, including the borderless resizable window, drag movement, F-key fullscreen toggle, configurable footer typography, MIDI output, automatic playback, drag-and-drop and command-line file opening.

The portable configuration file is SETTINGS.INI. The application version displayed in the footer is v0.2.
