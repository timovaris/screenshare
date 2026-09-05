# Publishing A Preview

## Before Publishing

1. Confirm the license and copyright attribution in `LICENSE`.
2. Run `scripts\build.cmd` and `scripts\smoke-test.ps1` on Windows.
3. Complete the applicable checks in [validation.md](validation.md), especially
   sharing the output to a second participant in at least one meeting app.
4. Review [README.md](../README.md), screenshots, and release limitations.
5. Run `powershell -ExecutionPolicy Bypass -File scripts\package.ps1` to create the
   portable ZIP and SHA-256 checksum in `dist`.

The current binary is an unsigned preview. Publish the ZIP and checksum as release
assets; keep generated binaries out of Git history. Update the CMake project,
manifest, version resource, About dialog, and changelog together for later versions.

## GitHub

Create a public GitHub repository named `regionshare` (or another chosen name).
Leave the remote empty because this project already contains its README and license.
Add its SSH or HTTPS URL as `origin` and push `main`.

The Windows workflow builds the executable, runs coordinate tests, and uploads the
portable package. It does not run GUI capture tests on the hosted CI runner and
does not automatically publish releases.

Create a **prerelease** for `v0.1.0-preview`, attach the ZIP and checksum, and use
the changelog entry as the release notes. Include the tested Windows, GPU, monitor,
and meeting-app configurations. Once a release exists, add its direct download
link near the top of the README.

Suggested repository description:

> Free, open-source Windows 11 region sharing with GPU capture and a normal shareable window.

Suggested topics: `windows`, `screen-sharing`, `screen-capture`, `cpp`, `win32`,
`direct3d11`, `windows-graphics-capture`.

## Next Development Priorities

1. Expand real meeting-app and mixed-DPI compatibility testing.
2. Remember window positions, region size, and preferences between sessions.
3. Improve single-monitor output placement and preset ergonomics.
4. Add keyboard shortcuts and accessibility improvements.
5. Measure GPU load and latency before adding FPS or scaling controls.
6. Consider signed binaries and an installer after the preview is proven useful.
