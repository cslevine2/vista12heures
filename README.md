# MIDI PIXEL

**MIDI PIXEL** is an experimental, lightweight MIDI player project.

Its central idea is to make MIDI activity visible as a compact, live pixel display: horizontal rows represent MIDI channels or file tracks, and the 128 MIDI note positions are laid out across each row. The display is intended to make the musical activity of a MIDI file immediately perceptible.

The project is being developed in public, with an emphasis on portability, clarity, documentation and a playful visual identity.

## Project repository

The GitHub repository is named **Vista12heures**.

The name is a wordplay around MIDI and its twelve musical note classes. The executable and product name remain **MIDI PIXEL**.

## Current status

The repository has just been created. This first commit establishes the project documentation and initial structure. There is not yet a complete player or a release executable.

See [the project overview](docs/PROJECT_OVERVIEW.md) for the intended features and [the development log](docs/DEVELOPMENT_LOG.md) for dated milestones.

## Development principles

- Keep the player lightweight and portable.
- Prefer a native Windows implementation, with Win32/GDI and Windows MIDI facilities as the initial technical direction.
- Keep user configuration accessible in external INI files where practical.
- Distinguish MIDI **tracks** from MIDI **channels**; they are not interchangeable.
- Document technical decisions, experiments, external references and contributions, including the use of AI tools.
- Preserve the project's playful details and easter eggs without making them prerequisites for understanding the software.

## Contributions and AI assistance

This project is human-directed and developed with AI assistance. The project documentation will identify AI contributions where relevant, including drafting, research, code suggestions, review and troubleshooting. The human maintainer remains responsible for decisions, testing, integration and releases.

## License

No software license has been selected yet. Until a license is added, the source code is not granted an open-source license by this repository. A license decision will be documented before public code distribution.
