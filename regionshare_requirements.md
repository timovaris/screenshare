# RegionShare (working title) — Requirements & Architecture (Windows 11)

## Goal

Provide a **universal “share a region”** capability on Windows 11 that works with **any meeting app** by exposing the selected region as a **regular window** that users can share via *Share window* in Teams/Zoom/Webex/Meet/etc.

Core idea:

1. User positions/sizes a dedicated **Share Window**
2. App **captures** the underlying screen content for that region (hardware-accelerated path)
3. App **renders** that region inside the Share Window
4. Meeting app shares the Share Window (no integration required)


## Non-goals (explicit)

- Not building plugins for specific meeting apps (Teams/Zoom/Webex…)
- Not implementing “share full screen” (meeting apps already do that)
- Not targeting Windows 10 in v1 (Windows 11 is sufficient)


## Technology stack (low-friction, native)

### Language & UI
- **C++ (MSVC)** + **Win32 windowing**
  - Rationale: lowest friction for D3D11 + Windows Graphics Capture, minimal dependencies, smallest runtime surface.

### Capture
- **Windows Graphics Capture (WGC)** (Windows 10 1903+; we target Win11)
  - Rationale: modern Windows capture API designed for better performance than older GDI-based approaches; integrates cleanly with D3D/DXGI pipelines.

### Rendering
- **Direct3D 11** + **DXGI swap chain** targeting the Share Window’s HWND
  - Rationale: common, stable, widely supported; straightforward pipeline for texture → crop/scale → present.

### Packaging & distribution (later)
- Start with a **portable executable** build.
- Consider **MSIX** or a simple installer later.
  - Rationale: keep MVP friction low; add polished distribution after core functionality is proven.


## Architecture overview

### Components
- **ShareWindow (UI)**
  - Borderless resizable window that the user will share.
  - Provides minimal controls (e.g., always-on-top, presets).
- **RegionModel**
  - Computes the capture region in **physical pixels** based on ShareWindow client rect, monitor DPI scaling, and monitor placement.
- **CaptureEngine (WGC)**
  - Captures the relevant monitor output and exposes frames as GPU textures.
- **Renderer (D3D11)**
  - Crops/scales the captured texture to the ShareWindow client size and presents.
- **HotkeyManager**
  - Optional global hotkeys for presets and window positioning.
- **Settings**
  - Stores last used presets, window position/size, monitor preference.

### Data flow (high level)
- Window move/resize → RegionModel updates region rect (screen px)
- CaptureEngine delivers frames (GPU texture)
- Renderer draws cropped region → swap chain present
- Meeting app shares ShareWindow


## MVP feature set (v0.1)

### 1) Share Window (the thing users share)
**Requirement**
- Provide a dedicated Share Window (borderless, resizable) that displays the chosen screen region.
- Must behave like a normal window so meeting apps can share it.

**Rationale**
- Universality: every meeting app supports “share window”.

---

### 2) Region = ShareWindow client area mapped to screen pixels
**Requirement**
- The captured region is defined by the ShareWindow’s client area position/size on the desktop.
- Moving/resizing the ShareWindow updates the region immediately.

**Rationale**
- Extremely intuitive UX.
- Avoids building a separate overlay region selector initially.
- Leverages Windows 11 Snap behavior “for free” (Win+Arrow, Win+Z).

---

### 3) Hardware-accelerated capture + render
**Requirement**
- Use WGC to acquire frames and keep the pipeline **GPU-side** where possible.
- Use D3D11 rendering into the ShareWindow swap chain.
- Provide a default cap of **30 FPS** (configurable later).

**Rationale**
- Keeps CPU low and latency acceptable on large/ultrawide screens.

---

### 4) DPI correctness (Windows 11, multi-monitor)
**Requirement**
- App must be **Per-Monitor DPI Aware (v2)**.
- RegionModel must convert correctly between:
  - window coordinates (DIPs)
  - desktop coordinates (physical pixels)
- Must work when monitors have different scaling factors.

**Rationale**
- Without this, cropped output will be offset/scaled incorrectly, especially on laptops + external monitors.

---

### 5) Basic controls
**Requirement**
- Toggle: **Always on Top**
- Button/menu: common **presets** (e.g., 1920×1080, 1280×720; plus “Half screen”)
- Option: “Lock capture to monitor where the ShareWindow currently is” (default behavior)

**Rationale**
- Always-on-top helps during presenting.
- Presets reduce fiddling and help create meeting-friendly aspect ratios.

---

### 6) Safe, predictable behavior
**Requirement**
- If capture fails (e.g., permissions, transient errors), show a clear status in the ShareWindow.
- Provide a “Pause” state when minimized (optional in MVP; acceptable as best-effort).

**Rationale**
- Presenting is time-critical; failures must be obvious and recoverable.


## Post-MVP enhancements (v0.2+)

### A) Preset system: grid & hot-zones (keyboard-first)
- Rich preset library:
  - fractions (1/2, 1/3, 2/3, 3/4)
  - fixed sizes (1920×1080, 2560×1440, 1600×900)
  - aspect lock (16:9, 21:9 crop-to-16:9)
- Optional global hotkeys for:
  - apply preset
  - move to next monitor
  - toggle always-on-top
- Optional integration with “FancyZones”-like layouts (not required).

**Rationale**
- Fast setup per meeting without mouse; consistent framing across customers.

---

### B) Cursor overlay options
- Render cursor on top of the captured output.
- Optional “highlight cursor” or click ripple.

**Rationale**
- Makes demos easier to follow, especially on scaled output.

---

### C) Capture source selection & resilience
- Manual monitor selection, “follow active monitor”, or “pin monitor” modes.
- Robust handling of display changes (dock/undock, resolution change).

**Rationale**
- Presenters often dock/undock laptops; capture must survive without restarts.

---

### D) Performance knobs
- FPS selection (15/30/60)
- Dynamic downscale for large regions
- Frame pacing / vsync options
- “Freeze last frame” on pause

**Rationale**
- Helps on lower-end GPUs and reduces bandwidth in meeting apps.

---

### E) Multi-window / multi-region
- Support multiple Share Windows simultaneously.

**Rationale**
- Some users want different framed outputs for different audiences or roles.

---

### F) Distribution & trust
- Code signing guidance
- Installer/MSIX packaging
- Auto-update (optional)

**Rationale**
- In enterprise environments, signed binaries reduce friction and support requests.


## UX notes (Windows 11)

- Users can rely on built-in snapping:
  - **Win+Arrow**: half/quarter screen placement
  - **Win+Z**: Snap Layouts picker
- MVP should not attempt to “intercept” Win+Arrow combos.
- Provide app-specific hotkeys/presets instead (to avoid conflicts).


## Quality attributes

- **Universal**: works with any meeting app via “share window”
- **Low CPU**: GPU-first pipeline
- **Low latency**: direct D3D path, minimal copies
- **Correct scaling**: per-monitor DPI aware
- **Robust**: clear error states, survives display changes
- **Simple UX**: “move/resize the Share Window” is the primary interaction


## Open questions (to resolve during implementation)

1. Cursor handling: include by default or optional?
2. Protected/DRM content behavior: document expected black-screen outcomes.
3. Permissions: confirm if any capture consent prompts are required in Win11 for this capture mode.
4. Preferred default presets for ultrawide users (e.g., default to 1920×1080 center).


## Suggested repository structure

- `/src`
  - `App/` (WinMain, message loop, window creation)
  - `ShareWindow/` (HWND wrapper, UI controls)
  - `RegionModel/` (DPI + coordinate transforms)
  - `CaptureEngine/` (WGC session, frame acquisition)
  - `Renderer/` (D3D11 device, shaders, swap chain)
  - `Hotkeys/` (optional)
  - `Settings/` (optional)
- `/docs`
  - `requirements.md` (this file)
  - `architecture.md` (later: diagrams, sequence flows)
- `/assets` (icons)


## Implementation milestones (suggested)

1. Create ShareWindow + D3D11 swap chain rendering a test pattern
2. Add WGC capture of a monitor and display live content in ShareWindow
3. Implement RegionModel crop based on ShareWindow client area
4. DPI correctness across two monitors with different scaling
5. Basic controls + presets
6. Stability / error handling polish
