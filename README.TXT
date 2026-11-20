MIDI PIXEL

MIDI PIXEL is a lightweight MIDI player whose visual concept represents MIDI notes as pixels across sixteen MIDI channel rows.

CURRENT BETA BUILD
VISTA12HEURES v0.1 reorganizes SETTINGS.INI and introduces section-independent key lookup. Every setting is found by its key name anywhere in the file, regardless of which [section] contains it. This allows users to reorganize the INI without changing application behavior, provided key names remain unique.

The configuration now uses explicit names for window geometry (windowX, windowY, windowWidth, windowHeight), velocity response (VelocityMinimumPercent, VelocityCurve), progress colors (ProgressBar, ProgressBackground, ProgressTopEdge), and octave guide colors (LinesMiddleC3color, LinesCOctavesCcolor). The application version is explicitly v0.1 in the footer and executable name.

Existing features from v0.07 are retained: borderless resizable window, drag movement from the lower area, F-key fullscreen toggle with restoration, configurable footer typography, once-per-second elapsed time, automatic playback after load, MIDI text information panel, command-line and drag-and-drop file opening, and MIDI output.

CONFIGURATION
SETTINGS.INI remains portable and user-facing. Its section names are organizational only: the application searches the whole file for each setting key. Keep each key unique to avoid ambiguity. Window geometry is saved back under [Interface] using windowX, windowY, windowWidth and windowHeight.

The local MIDI hardware selection remains a separate machine-specific configuration concern for a future step.

See docs/PROJECT_OVERVIEW.md for the intended design and docs/DEVELOPMENT_LOG.md for dated milestones.
