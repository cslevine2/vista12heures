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
