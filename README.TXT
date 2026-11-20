MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.107 adds MIDI Pitch Bend playback and compact per-track Volume/Pan indicators in TRACK MODE. Pitch Bend messages (0xE0) are forwarded to the MIDI output and reconstructed during seeking. TRACK MODE displays a Volume CC7 bar (default 100) and a Pan CC10 position marker (default center 64); the full-height Pan center reference line is configurable with verticalPanCenterLine in SETTINGS.INI. Loading a file in TRACK MODE no longer automatically enlarges the window; MIDI MODE retains its existing growth-on-load behavior.

BETA v0.106b added Standard MIDI File Control Change (CC) event playback. All channel Control Change messages are parsed and forwarded to the MIDI output at their scheduled times. When seeking, the player replays preceding Control Change events to reconstruct the synthesizer controller state. MIDI PIXEL does not assign musical meanings to individual controller numbers; the receiving synthesizer interprets them.
