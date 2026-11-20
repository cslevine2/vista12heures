MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.07 refines the borderless main window. The lower transport/footer area can be used as a drag surface to move the window; interactive controls retain their actions. The F-key fullscreen toggle now resizes the existing window to the active monitor bounds and restores its previous windowed coordinates and dimensions when toggled back.

The main piece title font size is configurable through Display/TrackTitleFontSize in SETTINGS.INI. The progress-bar filename and elapsed/total time are aligned using the actual selected font metrics, keeping the time display's right edge at a consistent margin.

Footer font sizes and RGB colors remain independently configurable in [Footer]. The main window remains borderless with an in-client X close control.

The elapsed mm:ss display refreshes once per second. Loading a MIDI file starts playback automatically from LOAD, command line or drag-and-drop. Command-line parsing attempts to reassemble unquoted file paths supplied by file managers.

The player loads Standard MIDI Files (SMF) Type 0 and Type 1, displays notes across sixteen MIDI channel rows, and sends MIDI events through the Windows MIDI output. The progress bar can be clicked to seek; active notes and channel programs are reconstructed at the selected position.

ERGONOMICS
A MIDI file can be opened by passing its path as a command-line argument (for example, through a Windows .MID file association using "%1") or by dropping it onto the application window. ESC closes the application. F toggles fullscreen. Keyboard shortcuts are configurable in [Keyboard] in SETTINGS.INI.

CONFIGURATION
All user settings are stored in one portable SETTINGS.INI beside VISTA12HEURES_v0.07.EXE. Sections cover window geometry, note colors, velocity response, interface colors, progress-zone height, display settings, keyboard shortcuts and footer typography.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
