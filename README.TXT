MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.04 rearranges the existing main-window elements into a cleaner composition. The sixteen channel rows occupy the upper visual area, with CH and PROGRAM labels beneath them. The piece title and track/resolution information are placed below the display; live tempo and time signature are grouped at lower left. The progress strip shows the MIDI filename at left and elapsed/total time at right in mm:ss. Transport controls are arranged with SETTINGS at left, LOAD and INFO near the center, and STOP and PLAY at right. The application name, version and authorship line appear at the bottom of the transport area.

The player loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends Note On, Note Off and Program Change messages to Microsoft GS Wavetable Synth through the native Windows MIDI API when available. Tempo and time-signature changes are displayed during playback. The progress bar is updated incrementally and can be clicked to seek; active notes and channel programs are reconstructed at the selected position. The INFO companion window displays MIDI text metadata and track names.

ERGONOMICS
A MIDI file can be opened by passing its path as a command-line argument (for example, through a Windows .MID file association using "%1") or by dropping it onto the application window. ESC closes the application. Keyboard shortcuts are configurable in [Keyboard] in SETTINGS.INI; the defaults are LEFT/RIGHT to seek by five seconds, SPACE for play/pause, and UP/DOWN for MIDI output volume.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.04.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, visible note range, guide-line thickness and keyboard shortcuts.

VERSIONING
Alpha prototypes used the form vA0.001, vA0.002, and so on. The functional BETA series begins at v0.01, followed by v0.02, v0.03, v0.04, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
