#include "RegionSelector.h"
#include <windowsx.h>
#include <string>

namespace RegShare {
int RegionSelector::Border() const { return MulDiv(5, GetDpiForWindow(hwnd_), 96); }
int RegionSelector::Header() const { return MulDiv(28, GetDpiForWindow(hwnd_), 96); }

bool RegionSelector::Create(HINSTANCE instance, HWND output, bool exclude_from_capture) {
  output_ = output;
  WNDCLASSW wc{};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = L"RegionShareSelector";
  if (!RegisterClassW(&wc)) return false;
  hwnd_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, wc.lpszClassName,
      L"RegionShare - Region", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480,
      nullptr, nullptr, instance, this);
  if (!hwnd_) return false;
  // Exclude only the selector. The output window must remain capturable.
  if (exclude_from_capture && !SetWindowDisplayAffinity(hwnd_, WDA_EXCLUDEFROMCAPTURE)) {
    Destroy();
    return false;
  }
  return true;
}
void RegionSelector::Destroy() {
  if (hwnd_) DestroyWindow(hwnd_);
  hwnd_ = nullptr;
}
void RegionSelector::Show(bool visible) {
  ShowWindow(hwnd_, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
}
void RegionSelector::SetRegion(PixelRect region) {
  SetWindowPos(hwnd_, nullptr, region.left - Border(), region.top - Header(),
      region.Width() + 2 * Border(), region.Height() + Header() + Border(),
      SWP_NOZORDER | SWP_NOACTIVATE);
}
PixelRect RegionSelector::Region() const {
  RECT r{};
  GetWindowRect(hwnd_, &r);
  return {r.left + Border(), r.top + Header(), r.right - Border(), r.bottom - Border()};
}
void RegionSelector::UpdateShape() {
  RECT r{};
  GetClientRect(hwnd_, &r);
  auto outer = CreateRectRgn(0, 0, r.right, r.bottom);
  auto inner = CreateRectRgn(Border(), Header(), std::max<LONG>(Border(), r.right - Border()),
                              std::max<LONG>(Header(), r.bottom - Border()));
  CombineRgn(outer, outer, inner, RGN_DIFF);
  DeleteObject(inner);
  if (!SetWindowRgn(hwnd_, outer, TRUE)) DeleteObject(outer);
  InvalidateRect(hwnd_, nullptr, FALSE);
}
LRESULT CALLBACK RegionSelector::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  auto* self = reinterpret_cast<RegionSelector*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    self = static_cast<RegionSelector*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
    self->hwnd_ = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  return self ? self->HandleMessage(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}
LRESULT RegionSelector::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_NCCALCSIZE: return 0;
    case WM_SIZE: UpdateShape(); return 0;
    case WM_DPICHANGED: {
      const RECT r = *reinterpret_cast<RECT*>(lp);
      SetWindowPos(hwnd_, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      UpdateShape();
      return 0;
    }
    case WM_GETMINMAXINFO: {
      auto* info = reinterpret_cast<MINMAXINFO*>(lp);
      info->ptMinTrackSize = {160, 120};
      info->ptMaxTrackSize = {8192, 8192};
      return 0;
    }
    case WM_NCHITTEST: {
      RECT r{};
      GetWindowRect(hwnd_, &r);
      const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp), b = Border();
      const bool left = x < r.left + b, right = x >= r.right - b;
      const bool top = y < r.top + b, bottom = y >= r.bottom - b;
      if (top) return left ? HTTOPLEFT : right ? HTTOPRIGHT : HTTOP;
      if (bottom) return left ? HTBOTTOMLEFT : right ? HTBOTTOMRIGHT : HTBOTTOM;
      if (left) return HTLEFT;
      if (right) return HTRIGHT;
      return HTCAPTION;
    }
    case WM_NCRBUTTONUP:
    case WM_CONTEXTMENU:
      PostMessageW(output_, WM_CONTEXTMENU, 0, lp);
      return 0;
    case WM_CLOSE: PostMessageW(output_, WM_CLOSE, 0, 0); return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC dc = BeginPaint(hwnd_, &ps);
      RECT r{};
      GetClientRect(hwnd_, &r);
      HBRUSH brush = CreateSolidBrush(RGB(16, 135, 104));
      FillRect(dc, &r, brush);
      DeleteObject(brush);
      SetBkMode(dc, TRANSPARENT);
      SetTextColor(dc, RGB(255, 255, 255));
      const auto region = Region();
      std::wstring title = L"RegionShare  |  " + std::to_wstring(region.Width()) +
                            L" x " + std::to_wstring(region.Height());
      r.left += Border() + 6;
      r.bottom = Header();
      auto old = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
      DrawTextW(dc, title.c_str(), -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
      SelectObject(dc, old);
      EndPaint(hwnd_, &ps);
      return 0;
    }
    case WM_ERASEBKGND: return 1;
  }
  return DefWindowProcW(hwnd_, msg, wp, lp);
}
}  // namespace RegShare
