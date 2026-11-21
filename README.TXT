MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.108 adds selectable Windows MIDI output devices. The SETTINGS window lists installed MIDI output ports, supports the Windows MIDI Mapper, and stores the selected destination separately in MIDIOUT.INI. Microsoft GS Wavetable Synth is the preferred default when available. TRACK MODE now visualizes Expression (CC11) instead of Volume (CC7); Pan (CC10) remains available. Volume is reserved for a future information panel.


BETA v0.2 adds three compact custom window controls (minimize, fullscreen toggle, close) and persists the fullscreen/windowed state in SETTINGS.INI as WindowsState. Window button color is configurable through WindowButtonsColor.


BETA v2.01 adds an optional compact MINI window. The standard NORMAL window remains the default and keeps its existing layout. MINI shows the sixteen MIDI channels as sixteen one-pixel guide lines, with active notes rendered as single pixels across the 128-note range. A NORMAL button beside the mini display restores the previous window size and position. The mode is temporary and does not alter the saved normal-window geometry.


v2.01 MINI layout refinement: MINI is the lower transport base of the original player, not a separate tiny floating window. It retains the original window width and bottom screen anchor; its top edge is calculated from the saved normal rectangle by subtracting ProgressHeight plus the fixed transport-zone height. The progress bar, transport commands, mini 16-channel display, NORMAL return button and footer remain together in this compact base.

 
VERSION HISTORY NOTE — The source and archive labels briefly used “v2.01” after v0.2. This was a numbering mistake during development, not a separate major-version branch. Development resumes with v0.202; the earlier archive filenames may be renamed locally to keep the user's archive sequence consistent.
 
v0.202: MINI display refinement. The compact transport base now shows only the 16 rows of active-note pixels (no guide lines and no NORMAL button). Clicking the mini display itself restores the full normal window. The original normal display remains unchanged.


v0.203: MINI channel order now matches the normal channel display (channel 16 at the top, channel 1 at the bottom). Active-note pixels refresh during playback in real time; the mini display and its progress/time strip are invalidated in their actual compact-window coordinates.


v0.204: MINI note updates now invalidate only the individual 1x1 note pixel, rather than the whole mini display, reducing frame and background flicker. Added the MidiLinearControlsColor RGB setting for the Expression (CC11) bars and Pan (CC10) indicators shown at the left of tracks in TRACK MODE.


v0.205: Corrected MINI pixel invalidation coordinates so the invalidated 1x1 rectangle exactly matches the drawn note pixel (including the one-pixel inset inside the mini frame). Vertically centered the mini display within the fixed transport zone.


v0.206: Added configurable SharpPixelShift vertical offset for sharp-note pixels (C#, D#, F#, G#, A#) in both MIDI and TRACK display modes. The default is 1 pixel; set SharpPixelShift=0 for the original aligned display. MINI display background now uses the same Background color as the main window.


v0.3: Added a reusable full-height side panel. Clicking the MIDI PIXEL footer label opens MIDIPIXELS.TXT, clicking the version opens history.TXT, and clicking cslevine+IA opens the looping auteurs.GIF animation. These files are read from the executable folder and remain externally editable. The panel width is configurable with InfoWindowWidth (default 350 pixels). Animated GIF playback uses Windows GDI+.
