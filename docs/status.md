# Project Status

## Starting Phase

Milestone 1 skeleton: ordinary Win32 window, D3D11 clear-color rendering, and a
Windows build script. No live capture, region geometry, DPI manifest, or controls.

## Current Phase

v0.1 preview candidate built and smoke-tested on Windows, with GitHub documentation
and portable packaging prepared. Meeting-app and hardware acceptance remain;
this is not yet a validated release.

| Milestone | Implementation | Acceptance |
| --- | --- | --- |
| 1. Window and D3D11 renderer | Flip-model output and selector | Windows Release build and pixel checks pass |
| 2. WGC monitor capture | Implemented | Live source pixel checks pass |
| 3. Region crop | GPU crop and letterboxing | Geometry tests and static recrop checks pass |
| 4. DPI and multiple monitors | PMv2, physical coordinates, monitor lock | Mixed-DPI hardware checks pending |
| 5. Controls and presets | Native menu, cursor, pause, topmost, presets | State toggles and 720p/1080p dimensions pass |
| 6. Failure handling | Status, retry, overlap pause, device/display recovery | Overlap recovery passes; device/display fault checks pending |

## Verification

- Built `build/windows/Release/RegShare.exe` with MSVC 19.44 and SDK 10.0.19041
  on Windows build 26200. The missing C++ compiler was installed into the existing
  Visual Studio 2022 Build Tools installation with approved elevation.
- Coordinate tests pass with both GCC on Linux and MSVC on Windows.
- `scripts/smoke-test.ps1` opens a known four-color source, compares output pixels,
  verifies changing frames and static recrops, and exercises capture/window states.
  It passed at 96 DPI, including pause/resume, overlap recovery, minimize/restore,
  topmost, cursor/monitor toggles, and fixed presets. Screenshots are in `build/smoke/`.
- WSL UNC builds emit MSBuild dependency-path casing warnings. The build succeeds;
  a checkout on a Windows filesystem avoids this incremental-build caveat.

## Working Decisions

- Separate excluded selector and capturable output, avoiding recursion. This
  departs from the original single-window positioning requirement and was raised
  with the user before implementation.
- Include cursor and lock monitor by default. Start with a region up to 1280 x 720,
  reserving same-monitor space for the output.
- Preserve the original requirements file; document implementation differences here.
- Defer settings persistence and all listed post-MVP enhancements.

## Public Preview Preparation

- Original multi-resolution icon, executable version metadata, About dialog,
  and explicit Pause/Resume labeling.
- Public-facing README with real screenshots using a synthetic sample document.
- MIT license, changelog, contribution guide, and publishing checklist.
- Local Git repository on `main`; generated builds and release ZIPs are ignored.
- Portable ZIP packaging includes license/documentation and a SHA-256 checksum.
- CI builds/tests and uploads the package; publishing a release remains manual.
- `--documentation` exposes the selector to screenshot tools for reproducible
  documentation only; normal launches retain capture exclusion.

## Remaining Release Gates

Complete the mixed-DPI, dock/undock, device-loss, performance, and meeting-app
checks in `validation.md`. Universal compatibility and production
readiness must not be inferred from the implemented code.
