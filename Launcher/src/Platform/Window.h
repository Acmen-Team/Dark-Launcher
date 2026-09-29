#pragma once

#include <Windows.h>

namespace Dark::Launcher
{
	// 无边框窗口。
	//
	// 非客户区被 WM_NCCALCSIZE 整个移除，标题栏与边框全部由 ImGui 绘制。
	// 但拖动、边缘缩放、Aero Snap、双击最大化仍然交回系统处理
	// （靠 WM_NCHITTEST 返回 HTCAPTION / HTLEFT 等），
	// 所以不需要手写 SetWindowPos 跟随鼠标的拖动逻辑。
	class Window
	{
	public:
		// 返回值表示消息是否已被处理，用于把消息转发给 ImGui。
		using MessageHook = bool (*)(HWND, UINT, WPARAM, LPARAM);

		// 以逻辑像素（100% DPI）给出期望尺寸，内部按显示器 DPI 换算。
		bool Create(const wchar_t* title, float logicalWidth, float logicalHeight);
		void Destroy();

		HWND  Handle() const { return m_Handle; }
		float DpiScale() const { return m_DpiScale; }

		// 注意：不能命名为 IsMaximized —— <windowsx.h> 里有一个同名的函数式宏，
		// 会把成员函数名直接展开掉。
		bool IsWindowMaximized() const;
		void Minimize();
		void ToggleMaximize();
		void RequestClose();

		// 取走 WM_SIZE 记录的待处理尺寸变化；没有变化时返回 false。
		bool ConsumeResize(unsigned& width, unsigned& height);

		void SetMessageHook(MessageHook hook) { m_MessageHook = hook; }

	private:
		static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		LRESULT HandleNcCalcSize(LPARAM lParam) const;
		LRESULT HandleNcHitTest(LPARAM lParam) const;
		LRESULT HandleGetMinMaxInfo(LPARAM lParam) const;

		void ApplyDwmChrome() const;

		HWND        m_Handle = nullptr;
		float       m_DpiScale = 1.0f;
		MessageHook m_MessageHook = nullptr;

		bool     m_ResizePending = false;
		unsigned m_PendingWidth = 0;
		unsigned m_PendingHeight = 0;
	};
}
