#pragma once

#include <Windows.h>

#include <d3d11.h>

namespace Dark::Launcher
{
	// 启动器自身的 D3D11 渲染上下文。
	// 只负责把 ImGui 的绘制数据呈现在窗口上，与引擎的渲染后端无关。
	class D3D11Context
	{
	public:
		bool Create(HWND hwnd);
		void Destroy();

		// 尺寸变化后重建交换链缓冲与渲染目标。宽高为 0 时忽略。
		void Resize(unsigned width, unsigned height);

		void BeginFrame(const float clearColor[4]);
		void Present(bool vsync);

		// 最近一次 Present 的返回码。失败时必须让上层知道，
		// 否则画面会停在最后一帧而看不出任何异常。
		HRESULT LastPresentResult() const { return m_LastPresentResult; }

		ID3D11Device*        Device() const { return m_Device; }
		ID3D11DeviceContext* DeviceContext() const { return m_Context; }

	private:
		bool CreateRenderTarget();
		void ReleaseRenderTarget();

		ID3D11Device*           m_Device = nullptr;
		ID3D11DeviceContext*    m_Context = nullptr;
		IDXGISwapChain*         m_SwapChain = nullptr;
		ID3D11RenderTargetView* m_RenderTarget = nullptr;
		HRESULT                 m_LastPresentResult = S_OK;
	};
}
