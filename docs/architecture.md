# RegionShare Architecture

## Window Model

The original milestone-1 skeleton only created a Win32 window and a D3D11 clear
color. The v0.1 implementation separates the movable **RegionSelector** from the
shareable **ShareWindow**. This is a working implementation decision, not a change
silently applied to the original requirements.

Windows display affinity is window-wide. Applying `WDA_EXCLUDEFROMCAPTURE` to the
output to avoid recursive monitor capture also excludes it from compatible meeting
capture APIs. Only the selector is excluded. Its window region is a hollow ring,
so the source remains visible and interactive. The output stays outside that ring;
intersection causes capture to stop and a visible pause status to replace it.

Reference: [SetWindowDisplayAffinity](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity).

## Capture And Rendering

- `CaptureEngine` creates a WGC monitor item with `CreateForMonitor`, wraps the
  renderer's DXGI device as a WinRT device, and uses a two-frame free-threaded pool.
- The UI thread polls frames on a timer, with a steady-clock presentation cap of
  30 FPS. No application frame callback uses the immediate D3D11 context.
- The renderer retains the latest monitor frame in its own GPU texture, freeing
  each WGC frame immediately. Static desktops can therefore be recropped without
  waiting for a new frame.
- `ComputeCrop` intersects physical desktop coordinates with monitor and texture
  bounds. A GPU copy fills the selected subrectangle of a black region texture.
- A fullscreen triangle samples that texture into an aspect-preserving viewport
  in a flip-discard BGRA8 swap chain. There is no CPU pixel readback.

The application is Per-Monitor DPI Aware v2 via manifest and startup API. Win32
screen/client geometry in this context is already in physical pixels. Only selector
border/header dimensions are converted from DIPs; source coordinates are never
scaled twice. Negative desktop origins are preserved.

## Lifecycle

Pause, minimize, and overlap close the capture session. Resume opens a new session
and does not display retained old pixels before receiving its first frame. Frame
size changes recreate the pool. Source closure uses a shared atomic flag, avoiding
callbacks into a destroyed UI object. Exceptions never escape the output WndProc.

Capture failures show their HRESULT/message and retry after three seconds. Device
removed/reset/hung errors request renderer recreation. Display configuration changes
reselect the nearest monitor and reset to a half-screen region. The monitor lock
defaults on and can be released through the native menu.

The executable is an unpackaged desktop application; it does not request silent
border removal or bypass capture policy. Exact consent and enterprise-policy
behavior must be tested on deployment Windows builds. WGC's normal capture border
remains enabled.

## Deferred

Persistent settings, global hotkeys, HDR tone mapping, multi-monitor compositing,
multiple output windows, signed distribution, and automatic updates are outside
this implementation. See `validation.md` for the hardware/meeting acceptance matrix.
