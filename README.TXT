MIDI PIXEL

MIDI PIXEL is an experimental, lightweight MIDI player project. Its visual concept represents MIDI notes as pixels across sixteen channel rows or, later, file-track rows.

CURRENT TEST BUILD
VISTA12HEURES_test vA0.012 is an ALPHA visual-interface prototype. It loads Standard MIDI Files (SMF) Type 0 and Type 1, reads note-on, note-off, program-change and tempo events, and visualizes active notes on sixteen MIDI channel rows. The build is visual-only: it does not produce MIDI sound.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_test_vA0.012.exe. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range and guide-line thickness, and separately colored octave-C markers including a dedicated C3 marker.

VERSIONING
Alpha GUI experiments use the form vA0.001, vA0.002, and so on.
The first playable MIDI-file build, with audible notes through a General MIDI sound source, will begin the BETA series: v0.1.00, v0.1.01, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.

vA0.011 — Consolidated all configuration into the root SETTINGS.INI. ProgressHeight controls the full progress-zone height, not just the fill. Display.FirstNote and LastNote define the visible pitch range, and Display.LineThickness controls channel guide-line width. The Windows build produces MIDI_PIXEL.exe, matching the source basename VISTA12HEURES_test_vA0.011.cpp.
