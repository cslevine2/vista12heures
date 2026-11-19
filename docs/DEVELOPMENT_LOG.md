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
