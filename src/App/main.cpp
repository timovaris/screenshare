#include <windows.h>
#include <winrt/base.h>

#include "ShareWindow/ShareWindow.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int show) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  try {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    RegShare::ShareWindow window(instance);
    const bool documentation = std::wstring_view(arguments) == L"--documentation";
    if (!window.Create(L"RegionShare - Share this window", documentation)) return 1;
    window.Show(show);
    return window.RunMessageLoop();
  } catch (const winrt::hresult_error& error) {
    MessageBoxW(nullptr, error.message().c_str(), L"RegionShare", MB_OK | MB_ICONERROR);
    return 1;
  } catch (...) {
    MessageBoxW(nullptr, L"RegionShare could not start.", L"RegionShare", MB_OK | MB_ICONERROR);
    return 1;
  }
}
