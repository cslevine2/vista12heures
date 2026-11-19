# MIDI PIXEL — project overview

## Purpose

MIDI PIXEL is conceived as a small MIDI player whose interface gives a visual, real-time representation of the notes being played.

The display is not intended to replace a piano roll or a full MIDI sequencer. It offers another perspective: a compact overview of musical activity as it happens.

## Main display concept

- A horizontal row represents one MIDI channel in channel mode.
- Each row spans all 128 MIDI note numbers (0–127).
- Currently active notes appear as small visual markers.
- Marker intensity is intended to reflect note velocity.
- Octave landmarks and C (DO) note positions should help orient the eye.
- The twelve pitch classes should be distinguishable by configurable colours.

Two display modes are planned:

1. **Channel mode:** sixteen rows, one for each MIDI channel.
2. **Track mode:** one row per MIDI file track.

A MIDI track and a MIDI channel are different concepts. A track may contain events for one or more channels, and several tracks may address the same channel. The implementation and interface must preserve this distinction.

## Appearance and configuration

The visual language is intended to be simple, compact and configurable.

- External INI files are planned for note colours and palettes.
- Multiple palettes should be able to coexist.
- Separate INI-based interface skins are envisaged.
- Candidate visual styles include dark, phosphor green, amber and retro-computer-inspired appearances.
- Resizing and zooming should remain comfortable and readable.

These are design intentions, not yet implemented features.

## Playback and information

The intended interface includes file information and transport controls, with a permanently visible progress indicator.

The proposed transport order is:

`STOP  PAUSE  BACKWARD  FORWARD  PLAY`

PLAY is intended to sit at the far right. Keyboard shortcuts and measure-based navigation are also planned. Measure navigation must take tempo and time-signature changes in the MIDI file into account rather than assuming a fixed number of seconds per measure.

Windows MIDI output is the initial platform direction. The exact output-routing options and any bundled synthesizer assets still require technical and licensing checks.

## Initial technical direction

The first implementation is expected to target modern Windows, using native Win32/GDI and Windows MIDI facilities where suitable. A lightweight, portable executable is a priority.

Older Windows compatibility, including Windows XP, may be investigated later as a separate compatibility effort. It is not a promise of the initial version.

## Historical and creative inspiration

The project draws visual inspiration from early computer-music interfaces, including the Vista Music Machine and its COMPOSE software. These references are inspirations; their original authorship, assets and licensing must be credited and checked carefully before any material is redistributed.

The project also embraces small jokes, hidden references and easter eggs. They are part of its personality, not required for operation.

## Open questions

- Final MIDI parsing and playback architecture.
- Output device selection and routing.
- Exact palette and skin formats.
- Track naming and display of MIDI text events / instrument names.
- License for the project source code.
- Whether optional additional features will be offered separately from the free base.
- Compatibility targets beyond the first modern Windows build.
