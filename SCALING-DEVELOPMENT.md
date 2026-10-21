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

## First live probe result — October 21, 12:36

All four F8 pipeline traces completed (1500 events each). The hangar main scene did not honor the requested 66.666667 screen percentage: both known TAA shaders (7DEC23DF and 640B9ECB), their source color, depth and motion-vector resources, and their outputs remained 3440x1440. No approximately 2294x960 scene/TAA target was observed. Existing 1720x720 and smaller resources are auxiliary passes and are insufficient evidence of lower-resolution main rendering.

The screenshots show the hangar and persistent description bars. This probe used native TAA, so it does not measure the released M color bridge. The improved released add-on and previous Engine.ini were restored after inspection. Need establish the effective scene-scale control (and whether hangar and flight use different rules) before implementing live scaling selection. Repeating the same hangar probe with the same INI value will not resolve that question.

## Effective scene-scale control investigation — October 21

Read-only inspection of the running game's config cache established the actual table:
`DrawScale=0..15` maps to `50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200` percent.
The sections are `GraphicsWindowsOption.DrawScale@N` in the effective Game.ini cache;
each contains a `DrawScale` string. Current `DrawScale=5` therefore selects 100 percent.

The apply path resolves this string and sets `r.ScreenPercentage` with priority
`0x09000000` (console), which can override the Engine.ini system-settings value.
Relevant preferred-image addresses (not ASLR process addresses): config read at
`0x1404651D6`, accessor at `0x1405F8780`, table lookup at `0x1405F87B0`,
apply call at `0x1405FA43A`, and console-variable setter at `0x1405F6B40`.
This explains why an INI-only request is insufficient, but does not prove how
hangar and flight use the resulting scale.

`scripts/diagnostics/read_scene_scale.py PID` reads only the target process and
reports both the cached mapping and current game/render-thread cvar values.
It requires no third-party Python packages, validates the executable hash and
loaded getter, bounds reads, and refuses unsupported executables. The validated
EXE SHA256 is `c7da97f5f8a807d4f1264adbb074146fcffe9bdc2ffa98791b822cd28e558f4f`.
Live smoke verification returned DrawScale 5, configured 100, and both cvar values
100.0. These values alone are not evidence of lower scene-rendering dimensions.

The on-disk `.text` is encrypted; direct disassembly/string-xref searches there
produce false results. Analysis used a local read-only snapshot of the loaded
code. Neither the game EXE nor process memory was modified. Do not commit or
distribute that snapshot.

Next: set the game's actual DrawScale to a reduced value with a config backup,
restart/apply, confirm the live cvar, and capture reduced scene/TAA dimensions in
hangar and flight. Only then select the correct downstream full-size handoff.

The development pipeline probe also accepts `AC7-DLSS-Captures/scaling-probe.request`.
It checks for this marker every 250 ms while idle and consumes it after opening
its trace. This trigger was compiled successfully; live marker-trigger testing
is pending. The newly built probe has not been installed over the good release.

## Scene-control and menu investigation — October 21, afternoon

The probe was subsequently installed temporarily with DrawScale 0 and the
Game.ini section selecting 50. Both live cvar copies reported 50.0. The file
trigger produced a complete 1500-event trace (`scaling-probe-pid7324-1.txt`),
validating that trigger. This trace was taken at the title screen, so it does
not establish reduced hangar or flight rendering. A 1720×720 viewport appears
in the trace but is not sufficient proof of the required main-scene path.

The user reported distorted/disappearing menus when changing Resolution
Scaling, including during the native-TAA probe. Therefore DLSS reconstruction
is not required for that symptom. The ultrawide log independently recorded
`15:25:04 F10: UI-plane composite hidden`, a debug toggle that can hide the
missing menu layer. The ultrawide candidate gates that toggle behind diagnostics
and recognizes reduced scene-shaped targets for menu correction. Its rendered
tests preserve glyph widths and positions at 50%, 58%, approximately 66.7%, and
native dimensions; in-game scaling-transition validation is pending.

`scripts/diagnostics/scene_scale_probe.py` prepares exact scene-percentage
requests through Game.ini and DrawScale, backs up both INIs, refuses to change
them while AC7 runs, and detects later edits during rollback. Fixture checks
passed for UTF-8, UTF-8 BOM, UTF-16, unrelated-setting preservation, missing
Game.ini, byte-for-byte restore, and refusal to overwrite later edits. AC7 can
remove the temporary Game.ini override on exit; the strict restore detected
that rewrite, and the actual diff was reviewed before restoring the originals.

The native-TAA probe and 50% config were removed after this test. The installed
Luma add-on again matches the released `06C797...A731` hash and DrawScale is 5.
ReShade/HDR settings were not changed. The separate ultrawide candidate is
installed locally with its original ASI and INI backed up. It is not a DLSS
scaling implementation or a public release update.
