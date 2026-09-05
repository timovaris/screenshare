#pragma once

#include <d3d11.h>
#include <atomic>
#include <memory>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

namespace RegShare {
class CaptureEngine {
 public:
  void Start(ID3D11Device* device, HMONITOR monitor, bool include_cursor);
  void Stop() noexcept;
  void SetCursor(bool include_cursor);
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame NextFrame();
  HMONITOR Monitor() const { return monitor_; }
  ~CaptureEngine() { Stop(); }
 private:
  HMONITOR monitor_ = nullptr;
  std::shared_ptr<std::atomic_bool> closed_;
  winrt::event_token closed_token_{};
  winrt::Windows::Graphics::SizeInt32 size_{};
  winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device_{nullptr};
  winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{nullptr};
  winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool_{nullptr};
  winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr};
};
}  // namespace RegShare
