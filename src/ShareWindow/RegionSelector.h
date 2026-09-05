#pragma once
#include <windows.h>
#include "RegionModel/RegionModel.h"

namespace RegShare {
class RegionSelector {
 public:
  bool Create(HINSTANCE instance, HWND output, bool exclude_from_capture = true);
  void Show(bool visible);
  void SetRegion(PixelRect region);
  PixelRect Region() const;
  HWND Handle() const { return hwnd_; }
  void Destroy();
 private:
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);
  void UpdateShape();
  int Border() const;
  int Header() const;
  HWND hwnd_ = nullptr;
  HWND output_ = nullptr;
};
}  // namespace RegShare
