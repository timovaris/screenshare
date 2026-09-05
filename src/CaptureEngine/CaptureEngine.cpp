#include "CaptureEngine.h"
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dxgi.h>

namespace RegShare {
using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;

void CaptureEngine::Start(ID3D11Device* device, HMONITOR monitor, bool include_cursor) {
  Stop();
  if (!GraphicsCaptureSession::IsSupported())
    throw hresult_error(E_NOTIMPL, L"Windows Graphics Capture is unavailable.");
  try {
    auto interop = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    check_hresult(interop->CreateForMonitor(monitor, guid_of<GraphicsCaptureItem>(), put_abi(item_)));
    closed_ = std::make_shared<std::atomic_bool>(false);
    closed_token_ = item_.Closed([closed = closed_](auto&&, auto&&) { closed->store(true); });
    com_ptr<IDXGIDevice> dxgi;
    check_hresult(device->QueryInterface(IID_PPV_ARGS(dxgi.put())));
    com_ptr<IInspectable> inspectable;
    check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(), inspectable.put()));
    device_ = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    size_ = item_.Size();
    pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(
        device_, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size_);
    session_ = pool_.CreateCaptureSession(item_);
    session_.IsCursorCaptureEnabled(include_cursor);
    session_.StartCapture();
    monitor_ = monitor;
  } catch (...) { Stop(); throw; }
}

void CaptureEngine::Stop() noexcept {
  try { if (item_ && closed_token_.value) item_.Closed(closed_token_); } catch (...) {}
  closed_token_ = {};
  try { if (session_) session_.Close(); } catch (...) {}
  try { if (pool_) pool_.Close(); } catch (...) {}
  session_ = nullptr;
  pool_ = nullptr;
  item_ = nullptr;
  device_ = nullptr;
  monitor_ = nullptr;
  closed_.reset();
}
void CaptureEngine::SetCursor(bool include_cursor) {
  if (session_) session_.IsCursorCaptureEnabled(include_cursor);
}
Direct3D11CaptureFrame CaptureEngine::NextFrame() {
  if (!pool_) return nullptr;
  if (closed_ && closed_->load())
    throw hresult_error(E_FAIL, L"The capture source was closed.");
  auto frame = pool_.TryGetNextFrame();
  if (!frame) return nullptr;
  const auto size = frame.ContentSize();
  if (size.Width <= 0 || size.Height <= 0) return nullptr;
  if (size.Width != size_.Width || size.Height != size_.Height) {
    frame.Close();
    size_ = size;
    pool_.Recreate(device_, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size_);
    return nullptr;
  }
  return frame;
}
}  // namespace RegShare
