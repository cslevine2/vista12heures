MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.03 adds a docked, borderless MIDI information panel, opened with INFO. The panel displays Standard MIDI File text metadata (text, copyright, instrument, lyrics, markers and cue points) together with all track names in their original order. It docks beside the main window, follows its position and size, and can be hidden with its small X. The progress fill is now aligned to the full progress band, with a two-pixel inset at both its top and bottom edges.

The player loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends Note On, Note Off and Program Change messages to Microsoft GS Wavetable Synth through the native Windows MIDI API when available. Tempo and time-signature changes are displayed during playback. The progress bar is updated incrementally and can be clicked to seek; active notes and channel programs are reconstructed at the selected position.

ERGONOMICS
A MIDI file can be opened by passing its path as a command-line argument (for example, through a Windows .MID file association using "%1") or by dropping it onto the application window. ESC closes the application. Keyboard shortcuts are configurable in [Keyboard] in SETTINGS.INI; the defaults are LEFT/RIGHT to seek by five seconds, SPACE for play/pause, and UP/DOWN for MIDI output volume.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.03.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range, guide-line thickness and keyboard shortcuts.

VERSIONING
Alpha prototypes used the form vA0.001, vA0.002, and so on. The functional BETA series begins at v0.01, followed by v0.02, v0.03, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
