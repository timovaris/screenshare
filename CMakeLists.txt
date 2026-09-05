cmake_minimum_required(VERSION 3.20)
project(RegShare VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
if(BUILD_TESTING)
  add_executable(RegionModelTests tests/RegionModelTests.cpp)
  target_include_directories(RegionModelTests PRIVATE src)
  add_test(NAME RegionModel COMMAND RegionModelTests)
endif()

if(NOT WIN32)
  message(STATUS "RegionShare requires Windows 11; building portable coordinate tests only.")
  return()
endif()
if(NOT MSVC)
  message(FATAL_ERROR "RegionShare requires MSVC and a Windows SDK with C++/WinRT (10.0.19041+).")
endif()
enable_language(RC)

add_executable(RegShare WIN32
  src/App/main.cpp
  src/App/RegShare.manifest
  src/App/RegShare.rc
  src/App/resource.h
  src/CaptureEngine/CaptureEngine.cpp
  src/CaptureEngine/CaptureEngine.h
  src/RegionModel/RegionModel.h
  src/ShareWindow/RegionSelector.cpp
  src/ShareWindow/RegionSelector.h
  src/ShareWindow/ShareWindow.cpp
  src/ShareWindow/ShareWindow.h
  src/Renderer/D3D11Renderer.cpp
  src/Renderer/D3D11Renderer.h
)

target_compile_definitions(RegShare PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
  _WIN32_WINNT=0x0A00 WINVER=0x0A00)
target_include_directories(RegShare PRIVATE src)
# SDK 19041's C++/WinRT headers require MSVC's legacy template lookup.
target_compile_options(RegShare PRIVATE /W4 /permissive /EHsc /utf-8 /await)
set_property(TARGET RegShare PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(RegShare PRIVATE
  d3d11
  dxgi
  d3dcompiler
  dxguid
  windowsapp
  dwmapi
)
