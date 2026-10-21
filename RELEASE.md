# v0.2.0-beta.1 — experimental DLSS scaling

Adds Quality, Balanced and Performance modes alongside native DLAA, using model preset M. The mode selector is in Home > Luma > AC7 DLSS Mode (experimental), and the choice is saved. Installer defaults to Quality and backs up the previous add-on, settings and scaling-marker state.

Live hangar captures at 3440×1440 confirm actual input/output dimensions for all four modes: Quality 2296×960, Balanced 1996×836, Performance 1720×720, and native DLAA 3440×1440, each outputting 3440×1440. Captured HDR-linear outputs contain no NaNs/infinities. Native TAA is bypassed after reconstruction warmup for scaling modes. GPU smoke tests pass for capture, context preservation, HDR conversion and TAA bypass copies.

This is an unofficial experimental prerelease. Flight, moving-camera quality, FPS gains, long sessions and other GPUs/displays are not validated. Existing cyan artifacts over aircraft-description text and highlight overshoot remain. DLSS follows reduced-resolution postprocessing. Some ultrawide configurations need a separate menu-scaling fix, which is not bundled. No frame generation.

Requires the matching official [Luma Unreal Engine latest-755 package](https://github.com/Filoppi/Luma-Framework/releases/download/latest-755/Luma-Unreal_Engine.zip), D3D11, an RTX GPU, and game TAA. The ZIP replaces the add-on and includes installer/restore scripts; NVIDIA DLLs, loader, game files and user captures are not included.

Add-on SHA256: `290DE5048C09954D4ADEA1C3F1E1FB666DB78A94F5418D42A5CE11F828A3ED56`.

See README.md and SCALING-CANDIDATE.md for installation and validation scope.
