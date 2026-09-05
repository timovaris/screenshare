#include "ShareWindow.h"
#include "App/resource.h"
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <sstream>

namespace RegShare {
namespace {
enum : UINT { kPause = 100, kRetry, kTopmost, kCursor, kPin, k720, k1080, kHalf, kExit, kAbout };
PixelRect Pixels(RECT r) { return {r.left, r.top, r.right, r.bottom}; }
MONITORINFO MonitorInfo(HMONITOR monitor) {
  MONITORINFO info{sizeof(info)};
  if (!GetMonitorInfoW(monitor, &info))
    throw winrt::hresult_error(E_FAIL, L"The selected monitor is no longer available.");
  return info;
}
}

ShareWindow::ShareWindow(HINSTANCE instance) : instance_(instance) {}

bool ShareWindow::Create(const wchar_t* title, bool documentation) {
  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = instance_;
  wc.lpszClassName = L"RegShareWindowClass";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_REGIONSHARE),
      IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
  wc.hIconSm = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_REGIONSHARE),
      IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
  if (!RegisterClassExW(&wc)) return false;
  menu_ = CreateMenu();
  options_ = CreatePopupMenu();
  AppendMenuW(options_, MF_STRING, kPause, L"Pause");
  AppendMenuW(options_, MF_STRING, kRetry, L"Retry capture");
  AppendMenuW(options_, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(options_, MF_STRING, kTopmost, L"Always on top");
  AppendMenuW(options_, MF_STRING, kCursor, L"Include cursor");
  AppendMenuW(options_, MF_STRING, kPin, L"Lock to current monitor");
  AppendMenuW(options_, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(options_, MF_STRING, k720, L"1280 x 720");
  AppendMenuW(options_, MF_STRING, k1080, L"1920 x 1080");
  AppendMenuW(options_, MF_STRING, kHalf, L"Half screen");
  AppendMenuW(options_, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(options_, MF_STRING, kAbout, L"About RegionShare");
  AppendMenuW(options_, MF_STRING, kExit, L"Exit");
  AppendMenuW(menu_, MF_POPUP, reinterpret_cast<UINT_PTR>(options_), L"Region");
  hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, title,
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 640, 420,
      nullptr, menu_, instance_, this);
  if (!hwnd_) { DestroyMenu(menu_); return false; }
  if (!selector_.Create(instance_, hwnd_, !documentation)) {
    MessageBoxW(hwnd_, L"The region selector could not be excluded from screen capture.",
                 L"RegionShare", MB_OK | MB_ICONERROR);
    DestroyWindow(hwnd_);
    return false;
  }
  pinned_monitor_ = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
  const auto work = MonitorInfo(pinned_monitor_).rcWork;
  // Reserve desktop space for the output on single-monitor systems.
  const int width = std::min(1280L, (work.right - work.left) * 2 / 3 - 24);
  const int height = std::min(720L, work.bottom - work.top - 80);
  selector_.SetRegion({work.left + 12, work.top + 40,
                       work.left + 12 + width, work.top + 40 + height});
  const int output_x = work.left + 24 + width;
  SetWindowPos(hwnd_, nullptr, output_x, work.top + 40,
      std::max(240L, work.right - output_x - 12), std::min(420L, work.bottom - work.top - 80),
      SWP_NOZORDER | SWP_NOACTIVATE);
  UpdateMenu();
  ready_ = true;
  SetTimer(hwnd_, 1, 10, nullptr);
  return true;
}
void ShareWindow::Show(int cmd_show) {
  ShowWindow(hwnd_, cmd_show);
  selector_.Show(!IsIconic(hwnd_));
  UpdateWindow(hwnd_);
}
int ShareWindow::RunMessageLoop() {
  MSG msg{};
  BOOL result;
  while ((result = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return result == -1 ? 1 : static_cast<int>(msg.wParam);
}
void ShareWindow::SetStatus(const std::wstring& status) {
  if (status == status_) return;
  status_ = status;
  if (!status.empty()) {
    // Remove stale shared pixels on pause/failure, even if the device was lost.
    try { renderer_.Clear(); } catch (...) {}
    SetWindowTextW(status_hwnd_, status.c_str());
    ShowWindow(status_hwnd_, SW_SHOW);
    InvalidateRect(status_hwnd_, nullptr, TRUE);
  } else {
    ShowWindow(status_hwnd_, SW_HIDE);
  }
}
void ShareWindow::Restart() {
  capture_.Stop();
  retry_after_ = {};
  received_frame_ = false;
}
void ShareWindow::Tick() {
  if (!ready_) return;
  const auto now = std::chrono::steady_clock::now();
  if (now - last_tick_ < std::chrono::microseconds(33334)) return;
  last_tick_ = now;
  if (paused_ || IsIconic(hwnd_)) {
    capture_.Stop();
    SetStatus(L"Paused");
    return;
  }
  const auto region = selector_.Region();
  RECT output{};
  if (FAILED(DwmGetWindowAttribute(hwnd_, DWMWA_EXTENDED_FRAME_BOUNDS, &output, sizeof(output))))
    GetWindowRect(hwnd_, &output);
  if (Overlaps(region, Pixels(output))) {
    capture_.Stop();
    SetStatus(L"Paused: output overlaps the capture region.");
    return;
  }
  if (now < retry_after_) return;
  if (rebuild_renderer_) {
    RECT client{};
    GetClientRect(hwnd_, &client);
    renderer_.Reset();
    renderer_.Initialize(hwnd_, client.right, client.bottom);
    rebuild_renderer_ = false;
  }
  const HMONITOR monitor = pin_ ? pinned_monitor_ :
      MonitorFromWindow(selector_.Handle(), MONITOR_DEFAULTTONEAREST);
  const auto info = MonitorInfo(monitor);
  if (!Overlaps(region, Pixels(info.rcMonitor))) {
    capture_.Stop();
    SetStatus(L"The region is outside the selected monitor.");
    return;
  }
  if (capture_.Monitor() != monitor) {
    capture_.Start(renderer_.Device(), monitor, cursor_);
    capture_started_ = now;
    received_frame_ = false;
    SetStatus(L"Starting capture...");
  }
  auto frame = capture_.NextFrame();
  if (!frame) {
    if (!received_frame_ && now - capture_started_ > std::chrono::seconds(5))
      throw winrt::hresult_error(E_FAIL, L"No capture frames received. Check Windows capture access.");
    if (received_frame_) renderer_.Render(nullptr, region, Pixels(info.rcMonitor));
    return;
  }
  auto access = frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
  winrt::com_ptr<ID3D11Texture2D> texture;
  winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(texture.put())));
  renderer_.Render(texture.get(), region, Pixels(info.rcMonitor));
  frame.Close();
  received_frame_ = true;
  SetStatus(L"");
}
void ShareWindow::ApplyPreset(int width, int height) {
  const HMONITOR monitor = pin_ ? pinned_monitor_ :
      MonitorFromWindow(selector_.Handle(), MONITOR_DEFAULTTONEAREST);
  const auto info = MonitorInfo(monitor);
  const auto work = Pixels(info.rcWork);
  if (!width) { width = work.Width() / 2; height = std::max(1, work.Height() - 40); }
  const int x = work.left + std::max(8, (work.Width() - width) / 2);
  const int y = work.top + std::max(32, (work.Height() - height) / 2);
  selector_.SetRegion({x, y, x + width, y + height});
}
void ShareWindow::UpdateMenu() {
  ModifyMenuW(options_, kPause, MF_BYCOMMAND | MF_STRING, kPause, paused_ ? L"Resume" : L"Pause");
  CheckMenuItem(options_, kTopmost, MF_BYCOMMAND | (topmost_ ? MF_CHECKED : MF_UNCHECKED));
  CheckMenuItem(options_, kCursor, MF_BYCOMMAND | (cursor_ ? MF_CHECKED : MF_UNCHECKED));
  CheckMenuItem(options_, kPin, MF_BYCOMMAND | (pin_ ? MF_CHECKED : MF_UNCHECKED));
}
void ShareWindow::Command(UINT command) {
  switch (command) {
    case kPause: paused_ = !paused_; Restart(); break;
    case kRetry: Restart(); break;
    case kTopmost:
      topmost_ = !topmost_;
      SetWindowPos(hwnd_, topmost_ ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      break;
    case kCursor: cursor_ = !cursor_; capture_.SetCursor(cursor_); break;
    case kPin:
      pin_ = !pin_;
      pinned_monitor_ = MonitorFromWindow(selector_.Handle(), MONITOR_DEFAULTTONEAREST);
      Restart();
      break;
    case k720: ApplyPreset(1280, 720); break;
    case k1080: ApplyPreset(1920, 1080); break;
    case kHalf: ApplyPreset(0, 0); break;
    case kAbout: {
      MSGBOXPARAMSW about{};
      about.cbSize = sizeof(about);
      about.hwndOwner = hwnd_;
      about.hInstance = instance_;
      about.lpszCaption = L"About RegionShare";
      about.lpszText = L"RegionShare 0.1.0 Preview\n\n"
                       L"Free, open-source region sharing for Windows 11.\n"
                       L"Capture and rendering stay on your computer.\n\n"
                       L"License and source information are included with the download.";
      about.dwStyle = MB_OK | MB_USERICON;
      about.lpszIcon = MAKEINTRESOURCEW(IDI_REGIONSHARE);
      MessageBoxIndirectW(&about);
      break;
    }
    case kExit: DestroyWindow(hwnd_); return;
  }
  UpdateMenu();
}
LRESULT CALLBACK ShareWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  auto* self = reinterpret_cast<ShareWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    self = static_cast<ShareWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
    self->hwnd_ = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
  try { return self->HandleMessage(hwnd, msg, wp, lp); }
  catch (const winrt::hresult_error& error) {
    if (msg == WM_CREATE) {
      MessageBoxW(hwnd, error.message().c_str(), L"RegionShare startup failed", MB_OK | MB_ICONERROR);
      return -1;
    }
    self->capture_.Stop();
    if (error.code() == DXGI_ERROR_DEVICE_REMOVED || error.code() == DXGI_ERROR_DEVICE_RESET ||
        error.code() == DXGI_ERROR_DEVICE_HUNG) self->rebuild_renderer_ = true;
    std::wostringstream text;
    text << L"Capture unavailable (0x" << std::hex << static_cast<unsigned long>(error.code().value)
         << L"). " << error.message().c_str();
    self->SetStatus(text.str());
    self->retry_after_ = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    return 0;
  } catch (...) {
    if (msg == WM_CREATE) return -1;
    self->capture_.Stop();
    self->SetStatus(L"Capture unavailable. Retry capture.");
    self->retry_after_ = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    return 0;
  }
}
LRESULT ShareWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_CREATE: {
      RECT r{};
      GetClientRect(hwnd, &r);
      renderer_.Initialize(hwnd, r.right, r.bottom);
      status_hwnd_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER,
          0, 0, r.right, r.bottom, hwnd, nullptr, instance_, nullptr);
      if (!status_hwnd_) return -1;
      SendMessageW(status_hwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
      SetStatus(L"Starting capture...");
      return 0;
    }
    case WM_TIMER: if (wp == 1) Tick(); return 0;
    case WM_COMMAND: Command(LOWORD(wp)); return 0;
    case WM_CONTEXTMENU: {
      POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
      if (p.x == -1 && p.y == -1) GetCursorPos(&p);
      TrackPopupMenu(options_, TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, nullptr);
      return 0;
    }
    case WM_SIZE:
      if (wp != SIZE_MINIMIZED) renderer_.Resize(LOWORD(lp), HIWORD(lp));
      if (status_hwnd_) MoveWindow(status_hwnd_, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
      if (ready_) selector_.Show(wp != SIZE_MINIMIZED);
      return 0;
    case WM_DPICHANGED: {
      auto r = *reinterpret_cast<RECT*>(lp);
      SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      return 0;
    }
    case WM_DISPLAYCHANGE:
      Restart();
      pinned_monitor_ = MonitorFromWindow(selector_.Handle(), MONITOR_DEFAULTTONEAREST);
      ApplyPreset(0, 0);
      return 0;
    case WM_GETMINMAXINFO:
      reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {240, 160};
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY:
      ready_ = false;
      KillTimer(hwnd, 1);
      capture_.Stop();
      selector_.Destroy();
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace RegShare
