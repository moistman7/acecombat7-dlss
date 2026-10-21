# v0.1.0-beta.1 — experimental DLAA/preset-M integration

Initial public AC7 DLSS project release, built on Luma latest-755.

- Native-resolution DLAA with model preset M.
- AC7-specific bounded color conversion to reduce colored noise on hangar rafters.
- D3D11 context-state preservation, F8 GPU capture, source overlay, and diagnostic tests.
- Installer with checksum validation and add-on/settings backup; restore script included.

**Quality, Balanced, Performance upscaling and frame generation are not included.**

Requires the matching official [Luma Unreal Engine latest-755 package](https://github.com/Filoppi/Luma-Framework/releases/download/latest-755/Luma-Unreal_Engine.zip). This ZIP replaces its add-on only; NVIDIA DLLs and the ReShade loader are not bundled.

Initial live testing: RTX 5090, 3440x1440. Hangar rafters improved substantially. Intermittent cyan description bars and highlight overshoot remain; flight, performance, and wider GPU compatibility still require testing. This is an unofficial experimental pre-release.

See README.md for installation, credits, known issues and rollback. Full source changes and build instructions are available in this repository. No game files or user captures are included.
