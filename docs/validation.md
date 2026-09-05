# Windows Acceptance Checks

Record Windows build, GPU/driver, monitor resolutions/scaling, and meeting-app
versions when running these checks. These are manual acceptance checks, not a
claim that the configurations have been tested.

## Capture And Geometry

- Put a sharp grid or text editor inside the selector. Verify every crop edge
  matches the outline's inner edge at 100%, 125%, 150%, and 200% scaling.
- Move and resize the selector over a static desktop. The crop must change even
  without new desktop animation. Resize the output and check aspect ratio.
- Place a second monitor left of and above the primary display. Repeat at mixed
  DPI, with monitor lock both on and off.
- Cross a monitor boundary: capture only the selected monitor, with black padding
  for the remainder. Unplug the selected display and verify recovery.
- Apply each preset; verify the selector reports the requested physical size.
- Test Win+Arrow on both windows. The standard output title bar supports Windows
  Snap Layouts; the borderless selector does not supply a custom Win+Z flyout.

## State And Failure

- Move the output into the selected region. Capture must stop and display the
  overlap status. Move it away and verify live capture resumes without recursion.
- Pause and minimize separately. Confirm previous shared pixels are cleared and
  restore/resume restarts capture. Toggle cursor and always-on-top.
- Close the output: the selector and capture session must also close.
- Change resolution, dock/undock, sleep/wake, and use Retry after failure.
- Test denied capture access and protected video. Failures should be visible;
  protected content may be black, not recoverable by retry.
- If practical, exercise device removal/reset. Recovery recreates the D3D device
  and swap chain, with capture restarted on that device.

## Meeting Apps And Performance

- In Teams, Zoom, Webex, and browser-based Meet, select the output using **Share
  window**, then verify motion, text, cursor, and pause status from a second client.
- Verify the selector is absent from captured content and the output is listed.
- Check behavior while the output is occluded and after minimizing/restoring.
- Measure CPU/GPU utilization and latency at 720p, 1080p, and ultrawide sizes.
  The UI timer caps presentation at 30 FPS; actual cadence can be lower. WGC may
  internally capture at the display rate; this is not a 30 FPS acquisition limit.
