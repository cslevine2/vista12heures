MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.05 refines the progress strip: filename and elapsed/total time text use a font size derived from ProgressHeight (ProgressHeight - 2, clamped to a readable range). The elapsed mm:ss display refreshes once per second during playback. Loading a MIDI file now starts playback automatically, whether the file is opened from the LOAD dialog, command line or drag-and-drop.

The main interface retains the sixteen channel rows, piece title, track/resolution information, live tempo and time signature, dockable INFO panel, and rearranged transport area.

The player loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends Note On, Note Off and Program Change messages to Microsoft GS Wavetable Synth through the native Windows MIDI API when available. The progress bar is updated incrementally and can be clicked to seek; active notes and channel programs are reconstructed at the selected position.

ERGONOMICS
A MIDI file can be opened by passing its path as a command-line argument (for example, through a Windows .MID file association using "%1") or by dropping it onto the application window. ESC closes the application. Keyboard shortcuts are configurable in [Keyboard] in SETTINGS.INI; the defaults are LEFT/RIGHT to seek by five seconds, SPACE for play/pause, and UP/DOWN for MIDI output volume.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.05.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range, guide-line thickness and keyboard shortcuts.

VERSIONING
Alpha prototypes used the form vA0.001, vA0.002, and so on. The functional BETA series begins at v0.01, followed by v0.02, v0.03, v0.04, v0.04c, v0.05, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
