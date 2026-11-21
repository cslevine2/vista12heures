MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.108 adds selectable Windows MIDI output devices. The SETTINGS window lists installed MIDI output ports, supports the Windows MIDI Mapper, and stores the selected destination separately in MIDIOUT.INI. Microsoft GS Wavetable Synth is the preferred default when available. TRACK MODE now visualizes Expression (CC11) instead of Volume (CC7); Pan (CC10) remains available. Volume is reserved for a future information panel.


BETA v0.2 adds three compact custom window controls (minimize, fullscreen toggle, close) and persists the fullscreen/windowed state in SETTINGS.INI as WindowsState. Window button color is configurable through WindowButtonsColor.


BETA v2.01 adds an optional compact MINI window. The standard NORMAL window remains the default and keeps its existing layout. MINI shows the sixteen MIDI channels as sixteen one-pixel guide lines, with active notes rendered as single pixels across the 128-note range. A NORMAL button beside the mini display restores the previous window size and position. The mode is temporary and does not alter the saved normal-window geometry.


v2.01 MINI layout refinement: MINI is the lower transport base of the original player, not a separate tiny floating window. It retains the original window width and bottom screen anchor; its top edge is calculated from the saved normal rectangle by subtracting ProgressHeight plus the fixed transport-zone height. The progress bar, transport commands, mini 16-channel display, NORMAL return button and footer remain together in this compact base.
