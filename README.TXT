MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.105 introduces two complementary instrument display modes. MIDI MODE displays the sixteen MIDI channels. TRACK MODE displays one row per MIDI file track, with each track name right-aligned in the label column immediately before its note line. The window grows vertically when needed to show larger arrangements.

In MIDI MODE, General MIDI channel 10 is labelled RYTHME and its notes use a dedicated percussion color. The percussion color is configurable with PercussionColor in SETTINGS.INI. The selected display mode is saved as instrumentsMidiMode=canal or instrumentsMidiMode=track.

The v0.103 centered Courier New title and metadata block, current Marker behavior, MIDI playback, seeking, file loading and other existing features are retained.


BETA v0.106b adds Standard MIDI File Control Change (CC) event playback. All channel Control Change messages are parsed and forwarded to the MIDI output at their scheduled times. When seeking, the player replays preceding Control Change events to reconstruct the synthesizer's controller state. MIDI PIXEL does not assign musical meanings to individual controller numbers; the receiving synthesizer interprets them.
