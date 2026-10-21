# AC7 scaling candidate — 2026-10-21

Local implementation: Quality, Balanced, Performance and Native DLAA. Open ReShade with Home, find Luma's **AC7 DLSS Mode (experimental)** selector. Keep DLSS selected and model preset M. Mode choice is saved in ReShade.ini under [Luma], AC7ScalingMode=0/1/2/3. The installed selection is Quality (1). HDR settings and the ultrawide menu fix are preserved.

Live hangar captures on the current PC, at 3440×1440:

| Mode | Actual NGX input | NGX output |
|---|---|---|
| Quality | 2296×960 | 3440×1440 |
| Balanced | 1996×836 | 3440×1440 |
| Performance | 1720×720 | 3440×1440 |
| Native DLAA | 3440×1440 | 3440×1440 |

All four produced nonblack input/output and no NaN/infinity half-float values. Dimensions reflect UE target alignment, not exact fractional arithmetic. Runtime diagnostics confirmed native TAA bypass after initial reconstruction warmup. Existing preset-M HDR color conversion is retained. Reduced-resolution postprocessing precedes DLSS at the final spatial-upscale pass; UI composition follows it.

Build and GPU smoke checks pass for capture readback, D3D11 state isolation, bounded HDR encode/decode, and TAA bypass copies, including odd dimensions, resizing, R11G11B10 input and RGBA16F input. Captures prove operation, not comparative quality or an FPS benefit. Flight, long sessions, other displays/GPUs, and motion quality remain unverified. Existing intermittent cyan artifacts over aircraft-description text remain; this is an experimental candidate, not a public release.

Installed final candidate SHA256: 290DE5048C09954D4ADEA1C3F1E1FB666DB78A94F5418D42A5CE11F828A3ED56.
The earlier live four-mode capture build was 5DED53A5F3E0551A2FA3EDF97574EFB1E2AEDFA87EE282670F0C83E31D1AA0DC. The final candidate adds saved mode selection and fixes compiler-blob lifetime in the tested TAA copy helper. The final installed build also produced a nonblack Quality hangar capture with no nonfinite values and native TAA bypass active. Live mode changes persist in ReShade.ini; HDR/display/preset M values remain unchanged.

The original native-DLAA add-on and scene-scale configurations are backed up locally. Recovery instructions accompany the installed candidate; close the game before restoring. Preserve the separate ultrawide menu fix and HDR settings.
