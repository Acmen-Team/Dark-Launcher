#include "Platform/D3D11Context.h"

#include <Windows.h>

namespace Dark::Launcher
{
	bool D3D11Context::Create(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount                        = 2;
		desc.BufferDesc.Width                   = 0;
		desc.BufferDesc.Height                  = 0;
		desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator   = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow                       = hwnd;
		desc.SampleDesc.Count                   = 1;
		desc.SampleDesc.Quality                 = 0;
		desc.Windowed                           = TRUE;
		desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

		const UINT flags = 0;
		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
		D3D_FEATURE_LEVEL level = {};

		HRESULT result = ::D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
			levels, _countof(levels), D3D11_SDK_VERSION,
			&desc, &m_SwapChain, &m_Device, &level, &m_Context);

		// 硬件设备不可用时退回 WARP 软件光栅化器。
		if (result == DXGI_ERROR_UNSUPPORTED)
		{
			result = ::D3D11CreateDeviceAndSwapChain(
				nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
				levels, _countof(levels), D3D11_SDK_VERSION,
				&desc, &m_SwapChain, &m_Device, &level, &m_Context);
		}

		if (FAILED(result))
			return false;

		return CreateRenderTarget();
	}

	void D3D11Context::Destroy()
	{
		ReleaseRenderTarget();
		if (m_SwapChain) { m_SwapChain->Release(); m_SwapChain = nullptr; }
		if (m_Context)   { m_Context->Release();   m_Context = nullptr; }
		if (m_Device)    { m_Device->Release();    m_Device = nullptr; }
	}

	bool D3D11Context::CreateRenderTarget()
	{
		ID3D11Texture2D* backBuffer = nullptr;
		if (FAILED(m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
			return false;

		const HRESULT result = m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
		backBuffer->Release();
		return SUCCEEDED(result);
	}

	void D3D11Context::ReleaseRenderTarget()
	{
		if (m_RenderTarget)
		{
			m_RenderTarget->Release();
			m_RenderTarget = nullptr;
		}
	}

	void D3D11Context::Resize(unsigned width, unsigned height)
	{
		if (!m_SwapChain || width == 0 || height == 0)
			return;

		ReleaseRenderTarget();
		m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget();
	}

	void D3D11Context::BeginFrame(const float clearColor[4])
	{
		m_Context->OMSetRenderTargets(1, &m_RenderTarget, nullptr);
		m_Context->ClearRenderTargetView(m_RenderTarget, clearColor);
	}

	void D3D11Context::Present(bool vsync)
	{
		m_LastPresentResult = m_SwapChain->Present(vsync ? 1 : 0, 0);
	}
}
