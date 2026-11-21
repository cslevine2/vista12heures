# MIDI PIXEL — development log

This file records durable project milestones. It is not intended to duplicate the full conversation history.

## 2026-11-16 — Project discussions begin

The MIDI PIXEL project discussion begins. The initial concept, visual direction, interface behaviour and technical possibilities are explored.

## 2026-11-19 — First public repository created

The public GitHub repository **Vista12heures** is created by the project maintainer:

https://github.com/cslevine2/vista12heures

The initial set of repository commits establishes the project documentation and directory structure.

## AI assistance and provenance

AI assistance is part of the project's development process. Relevant contributions may include brainstorming, technical research, drafting documentation, proposing code, reviewing code and helping diagnose problems.

The project maintainer directs the work, chooses what to retain, performs or arranges testing, and decides what is published. Specific AI contributions should be recorded as the work develops rather than retroactively attributing the entire project to an AI system.

## History archive

The maintainer plans to preserve selected conversation snapshots as standalone HTML files outside this repository, and include them in the eventual distribution archive. This avoids duplicating the complete conversation history here.

This log therefore records milestones and decisions only. The HTML archive, when prepared, will provide the fuller historical record.


## 2026-11-19 — Alpha GUI prototype vA0.002

The first visual prototype is revised as `VISTA12HEURES_test vA0.002`. The note area now uses clean horizontal blue channel guides without small per-note separators, and active demonstration notes are rendered as simple square pixels. This remains a visual-only alpha build: it does not load MIDI files or generate sound.

The project adopts an initial version convention: alpha GUI experiments use `vA0.001`, `vA0.002`, etc. The first playable MIDI-file build with audible General MIDI output will begin the beta series, initially `v0.1.00`, `v0.1.01`, etc.


## 2026-11-19 — Alpha GUI prototype vA0.004

The compact track-oriented display from vA0.003 is retained. The only visual adjustment in this iteration is the vertical placement of the demonstration note markers: they now sit above their horizontal guide lines instead of crossing them. The vA0.003 build also confirmed that the window geometry is restored from and saved to `SETTINGS.INI`.


## 2026-11-19 — Alpha GUI prototype vA0.005

The display now uses sixteen generic MIDI channels (01–16) instead of instrument names. Each channel exposes a placeholder GM-preset value, ready to become a program variable. Synthetic note markers animate independently on each row using a pseudo-random demonstration routine. The displayed `DEMO.MID` filename is still placeholder text; no MIDI file is loaded or played.


## 2026-11-19 — Alpha GUI prototype vA0.006

The random demonstration animation is replaced by a first Standard MIDI File reader. The prototype accepts SMF Type 0 and Type 1 files, retains track-level names and event origins internally, merges timed events for playback, and visualizes active notes by MIDI channel and pitch. Note-on and note-off events control marker visibility; program changes update the displayed GM preset number; tempo events contribute to the playback clock. The initial implementation is visual-only and does not yet send MIDI output to the Windows synthesizer.


## 2026-11-19 — Alpha GUI prototype vA0.007

Optimized painting: the static interface is rendered into a cached memory bitmap. During playback, note ON/OFF changes invalidate only the affected marker rectangles; the progress/status strip is refreshed separately. The source file `SOURCE/MIDI_PIXEL.cpp` is included in the downloadable build artifact together with the executable and `SETTINGS.INI`.


## 2026-11-19 — Alpha GUI prototype vA0.008

Three visual refinements: channel 16 is now at the top and channel 1 at the bottom; the visible horizontal note range is narrowed to MIDI note numbers 12–115 (outside notes remain parsed but are not displayed); marker brightness is mapped to NOTE ON velocity.


## 2026-11-19 — Alpha visual palette vA0.009

The visualizer now loads twelve user-editable note colors from CONFIG/COLORS.INI using decimal RGB triplets. Each chromatic note class keeps its color across all octaves. Note-on velocity controls brightness by scaling linear-light sRGB components, preserving hue and saturation. The palette also exposes a minimum brightness percentage and a velocity response curve.


## 2026-11-19 — Interface palette vA0.010

Added CONFIG/CouleursInterface.INI for independent customization of the main window background, horizontal channel lines, progress fill, transport panel surfaces and progress-bar thickness. The existing note palette remains separate in COLORS.INI.


## 2026-11-19 — Configuration and display geometry vA0.011

Consolidated the note palette, velocity response, interface colors, window geometry and display settings into one portable root-level SETTINGS.INI. ProgressHeight now controls the full progress-zone height. Added configurable visible MIDI note limits and channel guide-line thickness. The Windows build output is named MIDI_PIXEL.exe, matching the source basename MIDI_PIXEL.cpp.


## 2026-11-19 — Octave guide markers vA0.012

Added short markers directly over each channel guide line at every visible C note. The C3 marker (MIDI note 60) has its own configurable color (LinesC3color); all other octave-C markers use LinesCcolor. Marker width matches the four-pixel note marker and marker thickness follows LineThickness. The executable and source filenames return to the versioned VISTA12HEURES_test convention: VISTA12HEURES_test_vA0.012.exe and VISTA12HEURES_test_vA0.011.cpp.


## 2026-11-19 — INI marker-color fix vA0.013

Corrected the RGB reader so it receives the INI section explicitly. Interface colors continue to load from [Interface], while LinesC3color and LinesCcolor are now read from [Display]. Bumped the executable and displayed application version to vA0.013; source file is SOURCE/VISTA12HEURES_test_vA0.012.cpp.


## 2026-11-19 — GM program labels vA0.014

Added an external `GM-XG.XML` program-name table containing the 128 standard General MIDI Level 1 programs. The sixteen channel rows now display the current Program Change number (1–128) and its matching name when the XML table is available. The first Program Change encountered for each channel is shown immediately after loading; subsequent Program Change events update the display during playback. The XML file is optional, and the numeric program value remains visible without it. XG variation-bank selection is intentionally left for a later extension. The executable is `VISTA12HEURES_test_vA0.014.exe`; source is `SOURCE/VISTA12HEURES_test_vA0.013.cpp`.


## 2026-11-19 — Live tempo and time signature vA0.015

Added parsing of Standard MIDI File tempo meta-events (`FF 51`) and time-signature meta-events (`FF 58`). The interface displays the current tempo in BPM and the current time signature, updating both during playback when the MIDI file contains changes. If metadata is absent, the conventional defaults are 120 BPM and 4/4. The executable is `VISTA12HEURES_test_vA0.015.exe`; source is `SOURCE/VISTA12HEURES_test_vA0.014.cpp`.


## 2026-11-19 — First Windows MIDI output vA0.016

Added a first audible output path using the native Windows Multimedia MIDI API (WinMM). At startup, MIDI PIXEL searches for and opens Microsoft GS Wavetable Synth by name; if it is unavailable, the visual player remains usable and reports that audio output could not be opened. Note On, Note Off and Program Change events are sent through a small output layer that is independent of MIDI file parsing, leaving room for selectable hardware, virtual and other MIDI destinations later. STOP, pause, file replacement, end-of-file and application close send sustain-off and all-notes-off messages to avoid hanging notes. This is an initial output milestone: controllers, pitch bend, pressure, SysEx, bank selection and selectable output devices are not yet implemented. The executable is `VISTA12HEURES_test_vA0.016.exe`; source is `SOURCE/VISTA12HEURES_test_vA0.015.cpp`.


## 2026-11-19 — First functional BETA v0.01

The project moves from experimental ALPHA prototypes to its first functional BETA player. The executable naming convention becomes `VISTA12HEURES_v0.01.EXE`, with subsequent BETA versions continuing as `v0.02`, `v0.03`, etc. The progress bar now uses incremental invalidation during playback: only the newly advanced portion is redrawn rather than the entire bar. Clicking within the progress bar seeks to the corresponding playback time. Seeking reconstructs the active notes, current program on all channels, tempo and time signature from the MIDI event stream; if playback was active, it continues from the selected position. A seek intentionally refreshes the full progress region. The executable is `VISTA12HEURES_v0.01.EXE`; source is `SOURCE/VISTA12HEURES_v0.01.cpp`.


## 2026-11-19 — BETA v0.02 ergonomic controls

Added direct MIDI-file opening from the command line and drag-and-drop onto the application window. ESC now closes the application cleanly. Keyboard shortcuts are configurable in the [Keyboard] section of SETTINGS.INI, with defaults for relative seek, play/pause and MIDI output volume. The left/right seek interval is configurable in seconds. The progress fill is inset from its former dimensions for a cleaner appearance. The executable is VISTA12HEURES_v0.02.EXE; source is SOURCE/VISTA12HEURES_v0.02.cpp.


## 2026-11-20 — BETA v0.03 MIDI information panel

Added a borderless companion information window docked to the right of the main player. It follows the main window when moved or resized, can be shown or hidden with INFO, and closes via its small in-panel X. The panel lists MIDI text, copyright, instrument, lyric, marker and cue metadata, followed by track names in file order. Added UTF-8 text conversion with an ANSI fallback for legacy MIDI text. Refined the progress fill to span the full width of its background band while preserving a two-pixel inset at the top and bottom. Executable: VISTA12HEURES_v0.03.EXE; source: SOURCE/VISTA12HEURES_v0.03.cpp.


## 2026-11-20 — BETA v0.04 main-window layout

Rearranged the existing interface elements without introducing the next planned feature set. The sixteen channel rows move upward into the dedicated visual area, with CH and PROGRAM labels below them. The piece title and track/resolution information sit beneath the note display, while live tempo and time signature move to the lower-left information block. The progress strip displays the MIDI filename at left and elapsed/total time at right in mm:ss. SETTINGS, LOAD, INFO, STOP and PLAY are repositioned to match the revised transport layout; the application name, version and authorship line are placed at the bottom. The settings button is currently a visual placeholder. Executable: VISTA12HEURES_v0.04.EXE; source: SOURCE/VISTA12HEURES_v0.04.cpp.


## 2026-11-20 — BETA v0.04 layout correction

Corrected the note-marker row origin to match the raised channel guides, restoring each active note marker to its intended position immediately above its MIDI channel line. Separated the track-count/ticks-per-quarter line from the live tempo and time-signature lines in the lower-left information block. This is a corrective rebuild of BETA v0.04; no new features or configuration options were added.


## 2026-11-20 — BETA v0.04c micro-correction

Moved active note markers one additional pixel downward so they sit precisely against their channel guide lines. Repositioned the three lower-left information lines (track count and ticks per quarter, live tempo, time signature) using the progress-strip top edge as their vertical reference, keeping the complete block above the strip even when the window height changes. Updated metadata repaint invalidation to follow the same bottom-anchored region. This is a small corrective build; no new feature was added. Executable: VISTA12HEURES_v0.04c.EXE; source: SOURCE/VISTA12HEURES_v0.04c.cpp.


## 2026-11-20 — BETA v0.05 progress-strip refinements

The filename and elapsed/total time labels in the progress strip now use a font size derived from ProgressHeight (ProgressHeight minus two pixels, clamped for readability) and are vertically centered in the strip. The elapsed mm:ss value is refreshed once per second during playback. Loading a MIDI file now starts playback automatically for dialog, command-line and drag-and-drop opening paths. No MIDI parsing or event-timing behavior was changed. Executable: VISTA12HEURES_v0.05.EXE; source: SOURCE/VISTA12HEURES_v0.05.cpp.


## 2026-11-20 — BETA v0.06 window presentation and footer

Added independent font-size and RGB color settings for the application name, author credit and version labels in the [Footer] section of SETTINGS.INI. Removed the main window title bar and added a small in-client X close control. Added F-key fullscreen toggle, restoring the previous windowed position and size when leaving fullscreen. Improved command-line file argument handling by reassembling unquoted paths from file-manager arguments before attempting to open an existing file. The existing MIDI playback and display behavior is retained. Executable: VISTA12HEURES_v0.06.EXE; source: SOURCE/VISTA12HEURES_v0.06.cpp.


## 2026-11-20 — BETA v0.07 borderless-window refinements

Made the lower transport/footer area a drag surface for moving the borderless main window while preserving button actions. Reworked fullscreen to resize the existing window to the current monitor bounds rather than changing its window style, and restore the saved windowed rectangle on exit. Added configurable main piece-title font size through [Display] TrackTitleFontSize. Corrected elapsed/total time right alignment by measuring with the exact font used to draw the progress text. Executable: VISTA12HEURES_v0.07.EXE; source: SOURCE/VISTA12HEURES_v0.07.cpp.


## 2026-11-20 — BETA v0.1 SETTINGS.INI reorganization

Reorganized the portable SETTINGS.INI using the maintainer's new section layout. Replaced section-specific profile lookups with a key-based reader that searches the entire file, so a setting is recognized regardless of the section in which it appears. Updated the renamed keys: X/Y/Width/Height to windowX/windowY/windowWidth/windowHeight; MinimumPercent to VelocityMinimumPercent; Curve to VelocityCurve; Progress to ProgressBar; TransportTop to ProgressBackground; TransportEdge to ProgressTopEdge; LinesC3color to LinesMiddleC3color; and LinesCcolor to LinesCOctavesCcolor. The application version constant and footer display are explicitly set to v0.1, correcting the prior v0.07 source's stale v0.06 display label. Window geometry persistence now writes the new window* names under [Interface]. Executable: VISTA12HEURES_v0.1.EXE; source: SOURCE/VISTA12HEURES_v0.1.cpp.


## 2026-11-20 — BETA v0.2 piece metadata and bottom anchoring

Added main-display rendering for MIDI text meta-events 0x06 (Marker), 0x01 (Text Event) and 0x02 (Copyright), beneath the piece title and in their original file order. Other metadata remains available in the INFO window. Repositioned the piece title and selected metadata as one block calculated upward from the progress-strip boundary, so the block remains bottom-anchored as the window is resized taller and additional vertical room becomes available above it. Track-count and ticks-per-quarter information remains in the lower-left area. Executable: VISTA12HEURES_v0.2.EXE; source: SOURCE/VISTA12HEURES_v0.2.cpp.


## 2026-11-20 — BETA v0.102 current-marker display

Adjusted the post-v0.1 version sequence to v0.102, v0.103, etc. The main display now reserves exactly three metadata lines beneath the piece title: the most recent Marker (meta-event 0x06) at or before the current playback position, the first Text Event (0x01), and the first Copyright (0x02) event. Metadata events now retain their MIDI tick and are converted to microseconds using the file's tempo map. The current marker display updates when playback crosses a marker and naturally follows seeking. The title plus three lines remain bottom-anchored. The footer version constant is v0.102. Executable: VISTA12HEURES_v0.102.EXE; source: SOURCE/VISTA12HEURES_v0.102.cpp.


## 2026-11-20 — BETA v0.103 centered retro metadata block

Removed the visible labels "Marker", "Text Event" and "Copyright". The three fixed metadata slots now display only their corresponding text and remain blank when no matching event is present. The piece title and all three metadata slots are horizontally centered within the note timeline area. Added a Courier New fixed-pitch text renderer for this block to create a classic 8-bit composition-screen aesthetic. Existing bottom anchoring and current-marker timing behavior are retained. Footer version constant is v0.103. Executable: VISTA12HEURES_v0.103.EXE; source: SOURCE/VISTA12HEURES_v0.103.cpp.


## 2026-11-20 — BETA v0.104 MIDI channel and track display modes

Added the persistent `instrumentsMidiMode` setting (`canal` or `track`) and a two-line mode switch beneath SETTINGS. MIDI MODE retains the sixteen channel rows; General MIDI channel 10 is labelled RYTHME and uses an independently configurable `PercussionColor`. TRACK MODE displays one row per Standard MIDI File track, right-aligning each track name in the label column immediately before its guide line. Note activity is tracked by source track so the visualization reflects the file's track organization. The window grows vertically when a larger track count requires more room. Existing MIDI playback, seeking, and metadata display are retained. Executable: `VISTA12HEURES_v0.104.EXE`; source: `SOURCE/VISTA12HEURES_v0.104.cpp`.


## 2026-11-20 — BETA v0.105 TRACK startup and guide corrections

Fixed a startup crash when `instrumentsMidiMode=track` is saved: the static background renderer now accesses track names only after a MIDI file has actually been loaded, and falls back to the empty channel layout beforehand. Restored C3 and octave-C guide markers in TRACK MODE as well as MIDI MODE. Disabled automatic window growth when switching display modes; the existing growth-on-file-load behavior remains available for large arrangements. Executable: `VISTA12HEURES_v0.105.EXE`; source: `SOURCE/VISTA12HEURES_v0.105.cpp`.


## 2026-11-20 — BETA v0.106b MIDI Control Change playback

Restarted from the validated v0.105 source. Added parsing of channel Control Change messages (status 0xB0) in Standard MIDI Files and scheduled transmission through WinMM MIDI output. During seeking, prior Control Change events are replayed so the receiving synthesizer can reconstruct its controller state at the selected position. MIDI PIXEL forwards controller number/value pairs without hard-coding their musical interpretation; the destination synthesizer handles controller meanings such as filter cutoff or effects sends. The v0.105 interface, display modes, playback and seeking behaviour are otherwise retained. Executable: `VISTA12HEURES_v0.106b.EXE`; source: `SOURCE/VISTA12HEURES_v0.106b.cpp`.


## 2026-11-20 — BETA v0.107 Pitch Bend and TRACK controller meters

Added Standard MIDI File Pitch Bend channel events (status 0xE0), preserving the two seven-bit data bytes and forwarding them through WinMM at their scheduled positions. Seeking resets Pitch Bend to its neutral center on all channels, then replays preceding Pitch Bend and Control Change events to reconstruct the synthesizer state. TRACK MODE now includes compact per-track Volume (CC7) bars and Pan (CC10) position markers, initialized to the conventional defaults 100 and 64 and updated during playback and seeking. A single vertical Pan-center reference line spans the track stack and is configurable through verticalPanCenterLine in SETTINGS.INI. Track labels shift right to make room for the indicators. Automatic window growth on MIDI-file loading is now retained in MIDI MODE but disabled in TRACK MODE. Executable: VISTA12HEURES_v0.107.EXE; source: SOURCE/VISTA12HEURES_v0.107.cpp.


## 2026-11-20 — BETA v0.108 selectable MIDI output and Expression meter

Replaced the TRACK MODE Volume (CC7) mini-bar with an Expression (CC11) mini-bar, retaining Pan (CC10). Added a MIDI output Settings window using the same interface background color. It enumerates installed Windows MIDI output devices, offers the Windows MIDI Mapper as an optional destination, and applies the selected output immediately. The output choice is persisted separately in portable MIDIOUT.INI, leaving SETTINGS.INI dedicated to visual and player settings. Microsoft GS Wavetable Synth is selected by default when available; if unavailable, the first detected output is used, with MIDI Mapper as a final fallback. Volume (CC7) remains forwarded to the synthesizer but is no longer shown in TRACK MODE. Executable: VISTA12HEURES_v0.108.EXE; source: SOURCE/VISTA12HEURES_v0.108.cpp.


## 2026-11-20 — BETA v0.2 window controls and remembered display state

Added three compact window controls in the upper-right corner: minimize, fullscreen toggle (same monitor-filling behavior as the F key), and close. Their background color is configurable through `WindowButtonsColor` in SETTINGS.INI. The current display state is stored as `WindowsState` (`0` windowed, `1` fullscreen); the saved windowed position and dimensions remain intact while fullscreen is active, and fullscreen is restored on the next launch when requested. Executable: `VISTA12HEURES_v0.2.EXE`; source: `SOURCE/VISTA12HEURES_v0.2.cpp`.


## 2026-11-21 — BETA v2.01 compact MINI visualization

Added an optional MINI window mode, activated by a fourth custom window control. The normal player remains the default and retains its existing display. MINI uses the sixteen MIDI channel rows regardless of the selected normal display mode: each channel has a one-pixel horizontal guide, and each active MIDI note is represented by one pixel at its note-number position across the 128-note range. Note colors and percussion coloring are retained. A NORMAL button immediately beside the mini display restores the previous normal window rectangle. Entering MINI hides companion panels; the compact mode does not overwrite the saved normal-window geometry or fullscreen preference. Executable: `VISTA12HEURES_v2.01.EXE`; source: `SOURCE/VISTA12HEURES_v2.01.cpp`.


## 2026-11-21 — v2.01 MINI transport-base layout refinement

Refined MINI mode to retain the complete lower transport base of the normal player instead of opening a separate 210x52 window. The compact window keeps the saved normal window's left coordinate and bottom edge. Its height is `ProgressHeight + MINI_TRANSPORT_HEIGHT` (fixed transport zone: 54 px), so its top coordinate is `normalTop + normalHeight - miniHeight`. The progress strip, filename/time, SETTINGS, LOAD, INFO, STOP, PLAY and footer remain visible. The 128x34 mini channel display is integrated beside the transport controls, with a NORMAL button immediately beside it. Returning restores the exact saved normal rectangle. The normal view is otherwise unchanged.

 
## v0.202 — MINI pixel display refinement and version sequence correction
 
The preceding development files were temporarily labeled v2.01 by mistake after v0.2. This was a numbering error, not an intended major-version change. The sequence resumes at v0.202; the user will rename their local archive files as needed.
 
In MINI mode, removed the horizontal guide lines and the separate NORMAL button. The 16 MIDI channels remain represented by 16 distinct one-pixel rows; only active notes are drawn, one pixel per MIDI note, using the existing channel-note colors and dedicated percussion color. Clicking anywhere inside the mini display returns to the full normal window. The mini display was moved slightly upward within the transport base to avoid colliding with the footer. The normal-mode display and behavior are untouched.


## v0.203 — MINI channel order and live refresh

Reversed the vertical channel order in MINI so it matches the normal channel layout: MIDI channel 16 at the top and channel 1 at the bottom. Fixed live note refresh by invalidating the mini-note display rectangle when note-on/note-off events are processed; the previous invalidation used the full-size normal-view note coordinates, outside the compact window. Also corrected progress/time-strip invalidation to use the compact window's top coordinates, so its display remains live while playing.


## v0.204 — precise MINI pixel refresh and configurable linear-control color

Reduced MINI repainting to the exact 1x1 pixel affected by each Note On/Note Off event. This avoids repainting the mini display background and frame for every note transition and should eliminate the brief frame flicker seen during playback. Added the MidiLinearControlsColor RGB setting, loaded from SETTINGS.INI, to control the color of both the Expression (CC11) linear bars and Pan (CC10) position indicators at the left of tracks in TRACK MODE. Default: 205,150,55. The pan center reference line remains independently configurable through verticalPanCenterLine.


## v0.205 — MINI pixel refresh coordinate correction and vertical centering

Corrected the one-pixel invalidation Y coordinate: the drawn pixel is inset one pixel below the mini rectangle's top edge, and the invalidation now uses that exact same coordinate. This fixes the regression where note pixels appeared frozen until another repaint event. Centered the mini display vertically within the fixed 54-pixel transport zone using `(MINI_TRANSPORT_HEIGHT - MINI_SCREEN_H) / 2`; the drawing rectangle, hit-test rectangle and pixel invalidation all share this calculated anchor.


## v0.206 — sharp-note pixel offset and unified MINI background

Added the `SharpPixelShift` setting (default 1, clamped from 0 to 8 pixels) to raise the five sharp pitch classes (C#, D#, F#, G#, A#) slightly above natural-note pixels in both MIDI and TRACK modes. Setting it to 0 restores the original aligned rows. The shared `NoteRect` geometry applies the offset to drawing and live invalidation alike. MINI display background now uses the configured `Background` color, matching the main window. Executable: `VISTA12HEURES_v0.206.EXE`; source: `SOURCE/VISTA12HEURES_v0.206.cpp`.
