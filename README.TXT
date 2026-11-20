MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.06 adds independent font-size and RGB color settings for the three footer labels (application name, author credit and version) in [Footer] of SETTINGS.INI. The main window is now borderless, with a small in-client X control to close it. Press F to toggle fullscreen and press F again to restore the previous windowed size and position.

The progress strip retains its font size derived from ProgressHeight (ProgressHeight - 2, clamped to a readable range), and its elapsed mm:ss display refreshes once per second. Loading a MIDI file starts playback automatically from the LOAD dialog, command line or drag-and-drop. Command-line parsing also attempts to reassemble unquoted file paths supplied by file managers.

The main interface retains the sixteen channel rows, piece title, track/resolution information, live tempo and time signature, dockable INFO panel, and rearranged transport area.

The player loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends Note On, Note Off and Program Change messages to the selected Windows MIDI output (currently Microsoft GS Wavetable Synth). The progress bar is updated incrementally and can be clicked to seek; active notes and channel programs are reconstructed at the selected position.

ERGONOMICS
A MIDI file can be opened by passing its path as a command-line argument (for example, through a Windows .MID file association using "%1") or by dropping it onto the application window. ESC closes the application. F toggles fullscreen. Keyboard shortcuts are configurable in [Keyboard] in SETTINGS.INI; the defaults are LEFT/RIGHT to seek by five seconds, SPACE for play/pause, and UP/DOWN for MIDI output volume.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.06.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, display settings, keyboard shortcuts and footer typography.

VERSIONING
Alpha prototypes used the form vA0.001, vA0.002, and so on. The functional BETA series begins at v0.01, followed by v0.02, v0.03, v0.04, v0.04c, v0.05, v0.06, etc.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
