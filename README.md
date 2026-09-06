<p align="center">
  <img src="assets/regionshare.png" width="88" height="88" alt="RegionShare icon">
</p>

# RegionShare

**Share part of your screen through a normal window.**

RegionShare is a free, open-source Windows 11 utility. Position a region outline
over the content you want to present, then choose the RegionShare output in your
meeting app's **Share window** picker.

No meeting-app plugin, account, or subscription. RegionShare captures and renders
locally; your meeting app handles broadcasting.

![RegionShare showing a selected sample document and its live shareable output](docs/images/overview.png)

*Real application screenshot with a synthetic sample document. The green outline
selects the source; the separate output window shows the captured region.*

## Get RegionShare

Download the Windows 11 x64 portable ZIP from the
[**0.1.0 Preview release**](https://github.com/timovaris/screenshare/releases/tag/v0.1.0-preview),
or build from source using the instructions below.

Extract the ZIP and run `RegShare.exe`. Installation and administrator rights are
not required to run it. Preview binaries are unsigned.

This is an early preview. Local capture tests pass; broader meeting-app and
hardware compatibility results are tracked in
[GitHub Issues](https://github.com/timovaris/screenshare/issues).

## Use

1. **Frame the content.** Drag the green outline's header and resize its edges.
   Its hollow center lets you interact with the applications underneath.
2. **Place the output.** Keep the output window outside the selected region,
   either elsewhere on the same screen or on another monitor.
3. **Share the window.** In your meeting app, select
   **RegionShare - Share this window**.

Moving or resizing the outline changes the captured region. Resizing the output
preserves the source aspect ratio and adds black bars as needed.

### Controls

Open the **Region** menu, or right-click the outline's header.

| Control | Behavior |
| --- | --- |
| Pause / Resume | Stop capture and clear the output; resume live capture |
| Retry capture | Restart the capture session |
| Always on top | Keep the output above other windows |
| Include cursor | Include the mouse pointer; enabled by default |
| Lock to current monitor | Pin the source monitor; enabled by default |
| 1280 x 720 / 1920 x 1080 | Apply a fixed-size region in physical pixels |
| Half screen | Apply a region half the monitor work-area width |

<img src="docs/images/controls.png" width="480" alt="RegionShare's native Region menu with presets and capture controls">

Unlocking the monitor follows the monitor containing most of the outline.
Minimizing the output pauses capture and hides the outline; restoring resumes it.

## Why Two Windows?

A window placed over its own screen-capture region would capture itself.
Windows' capture-exclusion setting is window-wide: excluding that output would
also hide it from compatible meeting capture APIs.

RegionShare excludes only the selector and keeps the output shareable. If the
output overlaps the source, capture pauses automatically until they are separated.

## Features

- Native C++ and Win32 with Windows Graphics Capture and Direct3D 11.
- GPU capture, crop, and rendering without CPU pixel readback.
- Presentation capped at 30 FPS; actual cadence can be lower.
- Per-Monitor DPI Awareness v2 and physical-pixel region mapping.
- Visible error status, automatic retries, and overlap protection.
- Portable executable with no third-party runtime dependencies.

## Known Limitations

- Sharing uses the meeting app's existing window-capture feature. Compatibility
  with Teams, Zoom, Webex, and Meet has **not yet been validated**.
- Only one monitor supplies pixels. Areas outside it appear black. A full-monitor
  region needs room for the output on another monitor.
- Protected video may appear black. HDR tone mapping is not implemented;
  capture uses BGRA8 SDR. Enterprise policy can prevent capture.
- Preferences and window positions are not saved between sessions.

Track [meeting-app compatibility](https://github.com/timovaris/screenshare/issues/1),
[mixed-DPI and recovery validation](https://github.com/timovaris/screenshare/issues/2),
and [planned improvements](https://github.com/timovaris/screenshare/issues) in GitHub Issues.

## Build From Source

Prerequisites:

- Windows 11.
- Visual Studio 2022 or Build Tools with MSVC x64 C++ tools.
- Windows SDK 10.0.19041 or newer, including C++/WinRT.
- CMake 3.20 or newer.

From a Windows terminal in the checkout:

```bat
scripts\build.cmd
```

The script builds Release, runs coordinate tests, and produces
`build\windows\Release\RegShare.exe`. The MSVC runtime is linked statically.
For best incremental-build behavior, use a checkout on a Windows filesystem;
WSL UNC paths can produce MSBuild dependency-path casing warnings.

If an existing Build Tools installation is missing its compiler,
`scripts\install-build-tools.ps1` adds that component with administrator elevation.

Testing, packaging, and release instructions are in [CONTRIBUTING.md](CONTRIBUTING.md).

## Contribute

Compatibility reports are especially useful: include your Windows build, GPU,
monitor resolution/scaling, and meeting app. See [CONTRIBUTING.md](CONTRIBUTING.md)
for reporting bugs and submitting changes.

Use [GitHub Issues](https://github.com/timovaris/screenshare/issues) to report bugs,
discuss changes, and find work to contribute.

## License

[MIT](LICENSE). Free to use, modify, and redistribute, including commercial use,
subject to the license terms.
