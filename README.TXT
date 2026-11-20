MIDI PIXEL

CURRENT BETA BUILD
VISTA12HEURES v0.103 refines the bottom-anchored piece information block. The piece title and its three metadata slots (current Marker, first Text Event, first Copyright) are horizontally centered across the note timeline area. The labels "Marker", "Text Event" and "Copyright" are not displayed; each slot contains only its event text, and remains visually blank when no corresponding event exists. The title and metadata use Courier New, a fixed-width font, to evoke classic 8-bit composition displays.

The current Marker changes as playback crosses marker events and follows seeking. Text Event and Copyright display the first matching event in the MIDI file. The three metadata rows remain fixed in the layout, preserving a consistent composition whether or not the MIDI file contains those events.

Version sequence after v0.1 continues as v0.102, v0.103, etc. The footer displays v0.103.

All v0.102 functionality remains available, including section-independent SETTINGS.INI lookup, bottom anchoring, MIDI output, borderless resizable window, fullscreen toggle, auto-play, command-line and drag-and-drop loading.
