MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.102 displays a fixed three-line metadata block beneath the piece title:
- Marker: the most recent Marker meta-event (0x06) at or before the current playback position
- Text Event: the first Text Event meta-event (0x01) in the MIDI file
- Copyright: the first Copyright meta-event (0x02) in the MIDI file

Marker events are time-positioned using the MIDI tempo map, so the displayed marker changes as playback crosses marker positions and follows seeking. The title and the three metadata lines remain bottom-anchored above the progress strip.

Version numbering continues after v0.1 using v0.102, v0.103, and so on.

The section-independent SETTINGS.INI lookup introduced in v0.1 is retained: each setting is found by key name anywhere in the file, regardless of its [section]. Keep setting keys unique. All v0.1 and v0.2 functionality remains available, including the borderless resizable window, drag movement, F-key fullscreen toggle, configurable footer typography, MIDI output, automatic playback, drag-and-drop, command-line opening and INFO panel.

The portable configuration file is SETTINGS.INI. The application version displayed in the footer is v0.102.
