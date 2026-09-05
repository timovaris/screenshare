# Contributing

Bug reports, compatibility results, documentation improvements, and focused pull
requests are welcome.

Before filing an issue, check existing issues and include:

- RegionShare version and Windows build.
- GPU and driver, monitor resolution, and DPI scaling.
- Meeting app and version, if relevant.
- Reproduction steps, expected behavior, and what happened.
- The exact error message and a screenshot with private information removed.

For code changes, use the existing C++17/Win32/C++/WinRT structure and keep the
capture/render path on the GPU. Build with `scripts\build.cmd`; it also runs the
coordinate tests. For capture or window changes, run `scripts\smoke-test.ps1` on
Windows and record relevant manual checks from [validation](docs/validation.md).

Do not commit generated build output, private desktop screenshots, or credentials.
New dependencies and substantial UX changes should be discussed in an issue first.

The editable icon source is in `assets/regionshare.svg`. Documentation screenshots
can be reproduced with `scripts\capture-screenshots.ps1`, which uses a synthetic
sample document and closes its temporary windows when finished.
The script launches `RegShare.exe --documentation` so its outline is visible to
screenshot APIs. Normal launches always exclude the outline from capture.
