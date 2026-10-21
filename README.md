# ACE COMBAT 7 DLSS

Experimental DLSS integration for ACE COMBAT 7, built on the Luma Unreal Engine add-on.

**v0.2.0-beta.1 adds experimental Quality, Balanced and Performance upscaling, plus native DLAA.** Native DLAA works with any DLSS preset. M and K were the presets with reported issues; both are now fixed. The reduced-resolution scaling path in this binary still requires preset M. Frame generation is not included.

The installer selects Quality. Press **Home**, open Luma's settings, then use **AC7 DLSS Mode (experimental)**. The selected mode is saved. At 3440×1440, live hangar captures confirmed 2296×960 (Quality), 1996×836 (Balanced), 1720×720 (Performance), and 3440×1440 (native DLAA), all producing 3440×1440 output. See [validation and limitations](SCALING-CANDIDATE.md).

## What this release changes

An AC7-specific color conversion substantially reduces the green/purple speckling seen on hangar rafters with preset M. It maps HDR color into a bounded log+sRGB domain for DLSS and restores HDR-linear color afterward. HDR display output remains available. The patch also preserves D3D11 context state around evaluation and includes on-demand input/output capture.

The color conversion applies to preset M at native resolution and with the AC7 scaling path enabled. Other model presets retain the upstream color path. This is an experimental compatibility treatment, not an official NVIDIA or Luma release.

## Requirements

- ACE COMBAT 7 on Windows, using D3D11.
- NVIDIA RTX GPU supported by DLSS and a compatible driver. Initial live testing used an RTX 5090 at 3440x1440; other cards/resolutions are unverified.
- The exact **Luma Unreal Engine latest-755** package: [official release](https://github.com/Filoppi/Luma-Framework/releases/tag/latest-755), [Unreal Engine ZIP](https://github.com/Filoppi/Luma-Framework/releases/download/latest-755/Luma-Unreal_Engine.zip).
- Game TAA enabled. The patch replaces the detected TAA pass.

This download contains the modified add-on only. It does not bundle NVIDIA DLLs, the ReShade loader, game files, or the separate AC7 ultrawide mod. Existing live testing included that ultrawide mod; it is not declared a required dependency, but testing without it is pending.

## Install

1. Close the game. Back up your graphics loader, ReShade settings, and add-ons before setting up Luma.
2. Install the matching official Luma Unreal Engine package following its instructions. If you already have ReShade or other graphics injectors, follow Luma's compatibility guidance when setting it up.
3. Extract the AC7-DLSS release ZIP. In PowerShell, run its installer with your game directory:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1 -GameDirectory 'D:\YourSteamLibrary\steamapps\common\ACE COMBAT 7'
   ```

   The installer backs up the existing Luma add-on and ReShade.ini, checks the patch hash, replaces only the add-on, and selects DLSS/preset M and Quality, and activates the scaling marker. It retains your HDR/display settings and game configuration.
4. Enable TAA in the game. If TAA detection is absent, the tested Engine.ini settings are `[SystemSettings]` with `r.DefaultFeature.AntiAliasing=2` and `r.PostProcessAAQuality=4`; back up your config before changing it.
5. Launch the game and check the hangar, menus, bright scenes, and flight. Report the GPU, driver, resolution, display mode, and other graphics mods with issues.

To install manually, back up and replace `Luma-Unreal Engine.addon` beside `Ace7Game.exe`; select DLSS and preset M in Luma, and create an empty `ac7-scaling-experimental.enabled` file beside the game executable to activate the scaling selector. Keep the official package's shaders and NVIDIA DLL.

## Known limitations

- Intermittent cyan bars/blocks over aircraft-description text remain.
- Some bright pixels overshoot their input brightness in live captures. No NaNs/infinities appeared in the inspected corrected captures, but brightness is not fully validated.
- Hangar visuals improved in live testing; flight, performance, broad GPU compatibility, and long sessions are not yet validated.
- The mapping clamps negative HDR components to zero and components above 65504. It adds two compute passes and float16 conversions.
- No frame generation. Scaling is verified only on the tested game executable and hangar setup.
- DLSS follows low-resolution postprocessing in this UE4.18 integration; flight and moving-camera quality remain unverified.
- Existing ultrawide mods may need a separate menu fix when scene scaling changes; that mod is not bundled.

## Capture and rollback

Press **F8** in the game to capture two DLSS evaluations. This causes a readback stall and writes large raw files under `AC7-DLSS-Captures` beside the executable. Up to six requests are accepted per game process. Screenshots use Steam's normal F12 shortcut. F8 can also activate hotkeys in other installed mods.

The corrected captures include `original-input`, encoded `input-color`/`output-color`, and HDR-linear `decoded-output`, each with format/row-pitch metadata. `color-bridge-complete.txt` confirms the final decode stage. Capture files contain rendered image data; inspect them before sharing.

Close the game and run `Restore.ps1 -GameDirectory 'your game directory'` to restore the add-on and ReShade.ini from the latest installer backup. Restoring the INI also undoes changes made since that backup. Manual installs must restore their own backup.

To disable only the color conversion, create an empty `ac7-color-bridge.disabled` file beside `Ace7Game.exe` and restart. Remove it and restart to re-enable.

## Source, credits, and license

Based on Luma commit `4c7830b73bf76a670b5984e0fed3453ee7a239ab` (latest-755). See [BUILD.md](BUILD.md) for the exact source overlay and build steps.

Luma Framework by Filippo Tarpini (Pumbo), with its Unreal Engine integration by Izueh and Pumbo. AC7-specific compatibility changes and packaging: moistman7. ReShade by Patrick Mours and contributors. This software contains source code provided by NVIDIA Corporation.

Luma's [custom MIT license](LICENSE.md) requires preservation of its notices and author/project attribution; commercial usage requires the authors' permission. NVIDIA SDK components remain under their separate license in `licenses/NVIDIA-DLSS-LICENSE.txt`. Other dependency notices are retained under `licenses/`. No endorsement by Luma, NVIDIA, or the game's publisher is implied.
