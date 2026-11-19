MIDI PIXEL

MIDI PIXEL is an experimental, lightweight MIDI player project. Its visual concept represents MIDI notes as pixels across sixteen channel rows or, later, file-track rows.

CURRENT BETA BUILD
VISTA12HEURES v0.01 is the first functional BETA player. It loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends Note On, Note Off and Program Change messages to Microsoft GS Wavetable Synth through the native Windows MIDI API when available. Tempo and time-signature changes are displayed during playback. The progress bar is updated incrementally and can be clicked to seek; active notes and channel programs are reconstructed at the selected position.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.01.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range and guide-line thickness, and separately colored octave-C markers including a dedicated C3 marker.

VERSIONING
Alpha prototypes used the form vA0.001, vA0.002, and so on. The functional BETA series begins at v0.01, followed by v0.02, v0.03, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.

vA0.015 — Fixed INI section lookup so Display.LinesC3color and Display.LinesCcolor are read from the [Display] section. The dedicated C3 marker and the other octave-C markers now use their configured RGB values. The source is SOURCE/VISTA12HEURES_test_vA0.014.cpp. The display now shows each channel's current GM Program Change number and, when GM-XG.XML is present beside the executable, the corresponding General MIDI program name. The external XML table contains the 128 standard GM Level 1 programs; XG variation-bank support is a later extension.


vA0.015 — Displays the current tempo in BPM and time signature. Reads SMF tempo meta-events (FF 51) and time-signature meta-events (FF 58), including changes during playback. Defaults are 120 BPM and 4/4 when the file does not specify them. The source is SOURCE/VISTA12HEURES_test_vA0.014.cpp.


vA0.016 — First audible MIDI output stage. Uses the Windows Multimedia MIDI API (WinMM) and automatically opens Microsoft GS Wavetable Synth when present. Note On, Note Off and Program Change messages are sent in the existing playback event order. STOP, pause, file replacement, end-of-file and application close send sustain-off and all-notes-off messages to prevent stuck notes. If the synth is unavailable, the application remains usable in visual-only mode. The output layer is kept separate from MIDI event parsing so other output devices can be added later. The source is SOURCE/VISTA12HEURES_test_vA0.015.cpp.


v0.01 BETA — First functional MIDI player release. Adds incremental progress-bar drawing and click-to-seek. Seeking rebuilds the current channel programs, tempo, time signature and active-note state at the selected time, then resumes playback if playback was already running. The executable is VISTA12HEURES_v0.01.EXE; source is SOURCE/VISTA12HEURES_v0.01.cpp.
