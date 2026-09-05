#pragma once

#include <windows.h>

#include "Renderer/D3D11Renderer.h"
#include "CaptureEngine/CaptureEngine.h"
#include "RegionSelector.h"
#include <chrono>
#include <string>

namespace RegShare {

class ShareWindow {
 public:
  explicit ShareWindow(HINSTANCE instance);
  bool Create(const wchar_t* title, bool documentation = false);
  void Show(int cmd_show);
  int RunMessageLoop();

 private:
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  void Tick();
  void SetStatus(const std::wstring& status);
  void ApplyPreset(int width, int height);
  void Command(UINT command);
  void UpdateMenu();
  void Restart();

  HINSTANCE instance_;
  HWND hwnd_ = nullptr;
  HWND status_hwnd_ = nullptr;
  HMENU menu_ = nullptr;
  HMENU options_ = nullptr;
  D3D11Renderer renderer_;
  CaptureEngine capture_;
  RegionSelector selector_;
  HMONITOR pinned_monitor_ = nullptr;
  bool ready_ = false;
  bool paused_ = false;
  bool topmost_ = false;
  bool cursor_ = true;
  bool pin_ = true;
  bool received_frame_ = false;
  bool rebuild_renderer_ = false;
  std::wstring status_;
  std::chrono::steady_clock::time_point last_tick_{};
  std::chrono::steady_clock::time_point capture_started_{};
  std::chrono::steady_clock::time_point retry_after_{};
};

}  // namespace RegShare
