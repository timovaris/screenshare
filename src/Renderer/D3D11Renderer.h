#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include "RegionModel/RegionModel.h"

namespace RegShare {

class D3D11Renderer {
 public:
  void Initialize(HWND hwnd, UINT width, UINT height);
  void Resize(UINT width, UINT height);
  void Render(ID3D11Texture2D* source, PixelRect region, PixelRect monitor);
  void Clear();
  void Reset();
  ID3D11Device* Device() const { return device_.Get(); }

 private:
  void CreateRenderTarget();
  void CreateRegionTexture(UINT width, UINT height);

  UINT width_ = 0, height_ = 0;
  UINT region_width_ = 0, region_height_ = 0;

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> region_texture_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> latest_texture_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> region_rtv_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> region_srv_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
};

}  // namespace RegShare
