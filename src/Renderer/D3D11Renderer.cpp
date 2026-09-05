#include "D3D11Renderer.h"
#include <d3dcompiler.h>
#include <winrt/base.h>
#include <cstring>

namespace RegShare {
using Microsoft::WRL::ComPtr;
using winrt::check_hresult;
namespace {
constexpr char kShader[] = R"(
Texture2D screenTexture : register(t0);
SamplerState screenSampler : register(s0);
struct Vertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };
Vertex VS(uint id : SV_VertexID) {
  Vertex v;
  v.uv = float2((id << 1) & 2, id & 2);
  v.position = float4(v.uv * float2(2, -2) + float2(-1, 1), 0, 1);
  return v;
}
float4 PS(Vertex v) : SV_Target {
  return float4(screenTexture.Sample(screenSampler, v.uv).rgb, 1);
}
)";
constexpr float kBlack[] = {0, 0, 0, 1};
}

void D3D11Renderer::Reset() {
  if (context_) {
    context_->ClearState();
    context_->Flush();
  }
  *this = D3D11Renderer{};
}

void D3D11Renderer::Initialize(HWND hwnd, UINT width, UINT height) {
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
  check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
      device_.GetAddressOf(), nullptr, context_.GetAddressOf()));
  ComPtr<IDXGIDevice> dxgi;
  check_hresult(device_.As(&dxgi));
  ComPtr<IDXGIAdapter> adapter;
  check_hresult(dxgi->GetAdapter(&adapter));
  ComPtr<IDXGIFactory2> factory;
  check_hresult(adapter->GetParent(IID_PPV_ARGS(&factory)));
  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = std::max(1u, width);
  desc.Height = std::max(1u, height);
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  check_hresult(factory->CreateSwapChainForHwnd(device_.Get(), hwnd, &desc,
                                               nullptr, nullptr, &swap_chain_));
  check_hresult(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
  width_ = desc.Width;
  height_ = desc.Height;
  CreateRenderTarget();
  ComPtr<ID3DBlob> vs, ps, errors;
  check_hresult(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
      "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vs, &errors));
  errors.Reset();
  check_hresult(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
      "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &ps, &errors));
  check_hresult(device_->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(),
                                           nullptr, &vertex_shader_));
  check_hresult(device_->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(),
                                          nullptr, &pixel_shader_));
  D3D11_SAMPLER_DESC sample{};
  sample.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sample.AddressU = sample.AddressV = sample.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sample.MaxLOD = D3D11_FLOAT32_MAX;
  check_hresult(device_->CreateSamplerState(&sample, &sampler_));
  Clear();
}
void D3D11Renderer::Resize(UINT width, UINT height) {
  if (!swap_chain_ || !width || !height || (width == width_ && height == height_)) return;
  context_->OMSetRenderTargets(0, nullptr, nullptr);
  rtv_.Reset();
  check_hresult(swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
  width_ = width;
  height_ = height;
  CreateRenderTarget();
}
void D3D11Renderer::CreateRenderTarget() {
  ComPtr<ID3D11Texture2D> buffer;
  check_hresult(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&buffer)));
  check_hresult(device_->CreateRenderTargetView(buffer.Get(), nullptr, &rtv_));
}
void D3D11Renderer::CreateRegionTexture(UINT width, UINT height) {
  if (region_texture_ && width == region_width_ && height == region_height_) return;
  region_srv_.Reset();
  region_rtv_.Reset();
  region_texture_.Reset();
  D3D11_TEXTURE2D_DESC desc{};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
  check_hresult(device_->CreateTexture2D(&desc, nullptr, &region_texture_));
  check_hresult(device_->CreateRenderTargetView(region_texture_.Get(), nullptr, &region_rtv_));
  check_hresult(device_->CreateShaderResourceView(region_texture_.Get(), nullptr, &region_srv_));
  region_width_ = width;
  region_height_ = height;
}
void D3D11Renderer::Clear() {
  if (!rtv_) return;
  context_->ClearRenderTargetView(rtv_.Get(), kBlack);
  check_hresult(swap_chain_->Present(0, 0));
}
void D3D11Renderer::Render(ID3D11Texture2D* source, PixelRect region, PixelRect monitor) {
  if (source) {
    D3D11_TEXTURE2D_DESC incoming{}, cached{};
    source->GetDesc(&incoming);
    if (latest_texture_) latest_texture_->GetDesc(&cached);
    if (!latest_texture_ || incoming.Width != cached.Width || incoming.Height != cached.Height) {
      latest_texture_.Reset();
      incoming.BindFlags = incoming.MiscFlags = incoming.CPUAccessFlags = 0;
      incoming.Usage = D3D11_USAGE_DEFAULT;
      check_hresult(device_->CreateTexture2D(&incoming, nullptr, &latest_texture_));
    }
    // Own the retained texture so the WGC pool can immediately reuse its frame.
    context_->CopyResource(latest_texture_.Get(), source);
  }
  source = latest_texture_.Get();
  if (!source || !region.Width() || !region.Height()) return;
  CreateRegionTexture(region.Width(), region.Height());
  context_->ClearRenderTargetView(region_rtv_.Get(), kBlack);
  D3D11_TEXTURE2D_DESC desc{};
  source->GetDesc(&desc);
  auto crop = ComputeCrop(region, monitor, desc.Width, desc.Height);
  if (!crop.Empty()) {
    D3D11_BOX box{static_cast<UINT>(crop.source.left), static_cast<UINT>(crop.source.top), 0,
                  static_cast<UINT>(crop.source.right), static_cast<UINT>(crop.source.bottom), 1};
    context_->CopySubresourceRegion(region_texture_.Get(), 0, crop.destination_x,
                                    crop.destination_y, 0, source, 0, &box);
  }
  context_->ClearRenderTargetView(rtv_.Get(), kBlack);
  const float scale = std::min(static_cast<float>(width_) / region.Width(),
                               static_cast<float>(height_) / region.Height());
  D3D11_VIEWPORT viewport{};
  viewport.Width = region.Width() * scale;
  viewport.Height = region.Height() * scale;
  viewport.TopLeftX = (width_ - viewport.Width) / 2;
  viewport.TopLeftY = (height_ - viewport.Height) / 2;
  viewport.MaxDepth = 1;
  context_->RSSetViewports(1, &viewport);
  context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context_->VSSetShader(vertex_shader_.Get(), nullptr, 0);
  context_->PSSetShader(pixel_shader_.Get(), nullptr, 0);
  context_->PSSetSamplers(0, 1, sampler_.GetAddressOf());
  context_->PSSetShaderResources(0, 1, region_srv_.GetAddressOf());
  context_->Draw(3, 0);
  ID3D11ShaderResourceView* empty = nullptr;
  context_->PSSetShaderResources(0, 1, &empty);
  check_hresult(swap_chain_->Present(0, 0));
}
}  // namespace RegShare
