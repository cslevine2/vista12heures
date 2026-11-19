MIDI PIXEL

MIDI PIXEL is an experimental, lightweight MIDI player project. Its visual concept represents MIDI notes as pixels across sixteen channel rows or, later, file-track rows.

CURRENT TEST BUILD
VISTA12HEURES_test vA0.014 is an ALPHA visual-interface prototype. It loads Standard MIDI Files (SMF) Type 0 and Type 1, reads note-on, note-off, program-change and tempo events, and visualizes active notes on sixteen MIDI channel rows. The build is visual-only: it does not produce MIDI sound.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_test_vA0.014.exe. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range and guide-line thickness, and separately colored octave-C markers including a dedicated C3 marker.

VERSIONING
Alpha GUI experiments use the form vA0.001, vA0.002, and so on.
The first playable MIDI-file build, with audible notes through a General MIDI sound source, will begin the BETA series: v0.1.00, v0.1.01, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.

vA0.014 — Fixed INI section lookup so Display.LinesC3color and Display.LinesCcolor are read from the [Display] section. The dedicated C3 marker and the other octave-C markers now use their configured RGB values. The source is SOURCE/VISTA12HEURES_test_vA0.013.cpp. The display now shows each channel's current GM Program Change number and, when GM-XG.XML is present beside the executable, the corresponding General MIDI program name. The external XML table contains the 128 standard GM Level 1 programs; XG variation-bank support is a later extension.
