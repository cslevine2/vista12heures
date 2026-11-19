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
