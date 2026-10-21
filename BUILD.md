# Build the exact source

This repository is a small source overlay for Luma latest-755, not a complete Luma checkout. No personal settings, absolute developer paths, captures, game files, or NVIDIA runtime binaries are included.

1. Clone https://github.com/Filoppi/Luma-Framework with submodules.
2. Check out `4c7830b73bf76a670b5984e0fed3453ee7a239ab` and update submodules recursively.
3. Copy the contents of `source-overlay/` over that checkout, retaining the relative paths.
4. Build `Source/Games/Unreal Engine/Unreal Engine.vcxproj` using VS2022 Build Tools, v143, Windows SDK 10.0.26100.0, x64, Publishing-Release. Supply SolutionDir as the checkout's absolute path ending in a backslash, SolutionName=Luma, and LumaPackage=false.
5. The add-on is written to `Binaries/x64-Publishing-Release/Luma-Unreal Engine.addon`.

The overlay includes a WRL adapter used in place of ATL because the development toolchain lacked ATL. The custom MIT license remains applicable to Luma source and modifications; third-party SDK terms remain separate.

## Verification tools

From the overlaid Luma checkout:

    cmake -S Diagnostics -B Diagnostics/build -G "Visual Studio 17 2022" -A x64
    cmake --build Diagnostics/build --config Release

Run `capture-smoke.exe`, `state-smoke.exe`, `color-smoke.exe`, and `scaling-smoke.exe` from the Release directory. These test GPU readback, context restoration, and bounded color encode/decode (including odd dimensions, alpha, resizing, and finite FP16 range).

The `replay` target requires the matching official nvngx_dlss.dll beside replay.exe. It replays raw captures directly on the NVIDIA GPU. Basic usage:

    replay.exe CAPTURE_EVALUATION_DIRECTORY OUTPUT_DIRECTORY

It is a diagnostic harness for recorded color/depth/motion inputs, not a complete game-frame replay. It reads jitter from evaluation.json; each configuration creates a fresh DLSS feature and evaluates the same input eight times. Its history case is synthetic repeated-frame history, not the original moving-camera sequence. The zero-motion case explicitly uses a zero-filled texture because the SDK helper substitutes scale 1 when scale 0 is supplied. Additional arguments select an alternate input-color directory and a named test (for example M-bridge). See source for the tested parameter cases.

## Scaling candidate

The branch adds `ac7_scaling_runtime.hpp`, reduced-resolution reconstruction and saved mode selection. Create `ac7-scaling-experimental.enabled` beside Ace7Game.exe to activate the AC7-only path, then use the Luma mode selector with DLSS/preset M. The scaling smoke test validates the native-TAA bypass copy on WARP. See SCALING-CANDIDATE.md for live validation.

## Published binary

v0.1.0-beta.1 add-on SHA256:
06C7973BA017D04B9569CD16D924B732D1A5326FAB946B0AFDC6B0787365A731

Built with the earlier native-DLAA overlay; current branch additions produce the scaling candidate. A different toolchain or build path may produce different bytes. Verify behavior and third-party licenses when making your own release.
