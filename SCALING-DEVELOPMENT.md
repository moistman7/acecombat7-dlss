# Scaling work (unreleased)

The public v0.1.0-beta.1 tag is the tested native-resolution DLAA build. This branch is development only; do not package it as working scaling modes yet.

The color bridge now accepts different input/output dimensions. Offline GPU tests reconstructed synthetic reduced-resolution versions of the captured hangar scene:

| Mode target | Input | Output | Result |
|---|---|---|---|
| Quality | 2294x960 | 3440x1440 | NGX success, finite decoded HDR output |
| Balanced | 1996x836 | 3440x1440 | NGX success, finite decoded HDR output |
| Performance | 1720x720 | 3440x1440 | NGX success, finite decoded HDR output |

These inputs were downsampled from a native capture, with rescaled motion vectors and jitter. They validate the reconstruction path, not lower game-rendering cost or live temporal quality. The native-resolution color roundtrip smoke test still passes.

Remaining game work: reduce the actual scene-rendering cost, identify the low-resolution-to-output transition in AC7's pipeline, and route DLSS output through the correct downstream passes while preserving UI resolution, exposure, jitter, depth and motion coordinates. The upstream implementation targets a TAA output of matching size, so merely adding a mode dropdown or changing its ratio does not implement real upscaling.

## Temporary pipeline probe

A development add-on recognizes an `ac7-scaling-probe.enabled` marker beside Ace7Game.exe. It leaves TAA to the game and logs 1500 draws/dispatches when F8 is pressed. `AC7-DLSS-Captures/scaling-probe-pid*-*.txt` records shader hashes, viewport, output texture identity/dimensions/format, and the first eight input resources. There are no pixel readbacks from this trace. It can generate a short I/O stall.

The local test setup temporarily sets Engine.ini `r.ScreenPercentage=66.666667`; the configuration is not part of the published release and is backed up locally. This probe is intentionally not a completed Quality mode. The enabled marker must be removed and Engine.ini restored when returning to the released DLAA build. Initial low-resolution capture is pending.
