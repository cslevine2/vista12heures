MIDI PIXEL / Vista12heures

MIDI PIXEL is an experimental, lightweight MIDI player project. Its visual concept represents MIDI notes as pixels across sixteen channel rows or, later, file-track rows.

CURRENT TEST BUILD
VISTA12HEURES_test vA0.007 is an ALPHA visual-interface prototype. It loads Standard MIDI Files (SMF) Type 0 and Type 1, reads note-on, note-off, program-change and tempo events, and visualizes active notes on sixteen MIDI channel rows. The build is visual-only: it does not produce MIDI sound.

VERSIONING
Alpha GUI experiments use the form vA0.001, vA0.002, and so on.
The first playable MIDI-file build, with audible notes through a General MIDI sound source, will begin the BETA series: v0.1.00, v0.1.01, etc.
The final versioning convention can evolve as the project grows.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.


vA0.007 — The static interface is cached in an off-screen bitmap. MIDI note updates invalidate only the affected marker rectangles; the progress/status area is refreshed separately. The build artifact now includes SOURCE/MIDI_PIXEL.cpp alongside the executable and SETTINGS.INI.


vA0.008 — Channel rows are displayed in conventional order (MIDI channel 16 at top, channel 1 at bottom). The horizontal note range is focused on MIDI notes 12 through 115; notes outside that range are parsed and timed but not drawn. Marker brightness now follows note-on velocity.
