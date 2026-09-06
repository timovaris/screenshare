# Contributing

Bug reports, compatibility results, documentation improvements, and focused pull
requests are welcome.

Before filing an issue, check
[existing issues](https://github.com/timovaris/screenshare/issues) and include:

- RegionShare version and Windows build.
- GPU and driver, monitor resolution, and DPI scaling.
- Meeting app and version, if relevant.
- Reproduction steps, expected behavior, and what happened.
- The exact error message and a screenshot with private information removed.

## Development

Use the existing C++17/Win32/C++/WinRT structure and keep the capture/render path on
the GPU. `ShareWindow` owns the output and controls, `RegionSelector` defines the
physical-pixel source region, `CaptureEngine` manages WGC sessions, and
`D3D11Renderer` crops and presents captured textures. Coordinate calculations live
in `RegionModel` and can be tested without Windows.

Do not commit generated build output, private desktop screenshots, or credentials.
Discuss new dependencies and substantial UX changes in an issue first. Track
outstanding validation and feature work in GitHub Issues.

## Testing

Build and run coordinate tests on Windows with `scripts\build.cmd`. For capture or
window changes, also run the interactive desktop smoke test:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\smoke-test.ps1
```

Close other RegionShare instances first. The test uses a synthetic source and
checks live pixels, static recropping, pause/resume, overlap recovery,
minimize/restore, menu toggles, and fixed presets. Screenshots go to `build\smoke`.

Coordinate tests can also run on Linux:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Non-Windows builds only produce coordinate tests. CI builds and tests on Windows
but cannot replace interactive meeting-app or hardware testing. Record manual
results with Windows/app versions, GPU/driver, display layout, and scaling in the
relevant issue or pull request. The compatibility checklists are in
[issue #1](https://github.com/timovaris/screenshare/issues/1) and
[issue #2](https://github.com/timovaris/screenshare/issues/2).

## Documentation Assets

The editable icon source is in `assets/regionshare.svg`. Documentation screenshots
can be reproduced with `scripts\capture-screenshots.ps1`, which uses a synthetic
sample document and closes its temporary windows when finished.
The script launches `RegShare.exe --documentation` so its outline is visible to
screenshot APIs. Normal launches always exclude the outline from capture.

## Releases

Keep the version in CMake, the application manifest, Windows resource, About
dialog, and changelog consistent. Build and test the intended commit, then package:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package.ps1
```

The script writes a portable ZIP and SHA-256 checksum to `dist`. CI also uploads
these as build artifacts. Attach both files to a GitHub release whose tag matches
the built commit. Use the changelog for release notes and identify preview releases
as prereleases. Keep generated binaries out of Git history.
