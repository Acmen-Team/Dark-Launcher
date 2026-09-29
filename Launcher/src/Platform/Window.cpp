#include "Platform/Window.h"

#include "Platform/WindowChrome.h"

#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM

namespace Dark::Launcher
{
	namespace
	{
		constexpr const wchar_t* kWindowClassName = L"DarkLauncherWindow";

		// DWM 圆角 / 边框颜色属性：Windows 11 才支持，旧系统调用返回失败，忽略即可。
		constexpr DWORD kDwmWindowCornerPreference = 33;
		constexpr DWORD kDwmBorderColor            = 34;
		constexpr int   kDwmCornerRound            = 2;

		// 对应 dwmapi.h 的 DWMWA_COLOR_NONE。这里动态加载 dwmapi，
		// 不引入该头文件，所以自行声明。
		constexpr COLORREF kDwmColorNone = 0xFFFFFFFE;

		using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);

		DwmSetWindowAttributeFn DwmSetWindowAttributeProc()
		{
			static const DwmSetWindowAttributeFn proc = []() -> DwmSetWindowAttributeFn
			{
				HMODULE module = ::LoadLibraryW(L"dwmapi.dll");
				if (!module)
					return nullptr;
				return reinterpret_cast<DwmSetWindowAttributeFn>(
					::GetProcAddress(module, "DwmSetWindowAttribute"));
			}();
			return proc;
		}
	}

	bool Window::Create(const wchar_t* title, float logicalWidth, float logicalHeight)
	{
		const HINSTANCE instance = ::GetModuleHandleW(nullptr);

		WNDCLASSEXW wc = {};
		wc.cbSize        = sizeof(wc);
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc   = &Window::WndProcThunk;
		wc.hInstance     = instance;
		wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
		wc.hIcon         = ::LoadIconW(nullptr, IDI_APPLICATION);
		wc.hbrBackground = nullptr;   // 背景由 D3D11 负责，不做擦除以避免闪烁
		wc.lpszClassName = kWindowClassName;

		if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			return false;

		const float systemScale = static_cast<float>(::GetDpiForSystem()) / 96.0f;
		const int width  = static_cast<int>(logicalWidth * systemScale + 0.5f);
		const int height = static_cast<int>(logicalHeight * systemScale + 0.5f);

		int x = CW_USEDEFAULT;
		int y = CW_USEDEFAULT;
		RECT work = {};
		if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0))
		{
			x = work.left + ((work.right - work.left) - width) / 2;
			y = work.top + ((work.bottom - work.top) - height) / 2;
		}

		// 保留 WS_OVERLAPPEDWINDOW：它带来系统阴影、任务栏行为、边缘缩放与 Aero Snap。
		// 视觉上的边框/标题栏会在 WM_NCCALCSIZE 里被整个去掉。
		m_Handle = ::CreateWindowExW(
			0, kWindowClassName, title, WS_OVERLAPPEDWINDOW,
			x, y, width, height,
			nullptr, nullptr, instance, this);

		if (!m_Handle)
			return false;

		// CreateWindowExW 期间系统只发一次 wParam == FALSE 的 WM_NCCALCSIZE，
		// 那次并不负责重算非客户区，所以窗口刚创建时仍然带着原生边框。
		// SWP_FRAMECHANGED 会强制补发一次 wParam == TRUE 的 WM_NCCALCSIZE，
		// 那才是真正把非客户区去掉的时机。
		::SetWindowPos(m_Handle, nullptr, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

		m_DpiScale = static_cast<float>(::GetDpiForWindow(m_Handle)) / 96.0f;
		ApplyDwmChrome();

		// CreateWindowExW 不会自动显示窗口，必须显式 ShowWindow，
		// 否则窗口不可见、WM_SIZE 也不会派发。
		::ShowWindow(m_Handle, SW_SHOWDEFAULT);
		::UpdateWindow(m_Handle);
		return true;
	}

	void Window::Destroy()
	{
		if (m_Handle)
		{
			::SetWindowLongPtrW(m_Handle, GWLP_USERDATA, 0);
			::DestroyWindow(m_Handle);
			m_Handle = nullptr;
		}
		::UnregisterClassW(kWindowClassName, ::GetModuleHandleW(nullptr));
	}

	void Window::ApplyDwmChrome() const
	{
		const DwmSetWindowAttributeFn setAttribute = DwmSetWindowAttributeProc();
		if (!setAttribute)
			return;

		const int corner = kDwmCornerRound;
		setAttribute(m_Handle, kDwmWindowCornerPreference, &corner, sizeof(corner));

		// 去掉系统绘制的细边框，避免与自绘外壳出现双重描边。
		const COLORREF noBorder = kDwmColorNone;
		setAttribute(m_Handle, kDwmBorderColor, &noBorder, sizeof(noBorder));
	}

	bool Window::IsWindowMaximized() const
	{
		return m_Handle && ::IsZoomed(m_Handle) != FALSE;
	}

	void Window::Minimize()
	{
		if (m_Handle)
			::ShowWindow(m_Handle, SW_MINIMIZE);
	}

	void Window::ToggleMaximize()
	{
		if (!m_Handle)
			return;
		::ShowWindow(m_Handle, ::IsZoomed(m_Handle) ? SW_RESTORE : SW_MAXIMIZE);
	}

	void Window::RequestClose()
	{
		if (m_Handle)
			::PostMessageW(m_Handle, WM_CLOSE, 0, 0);
	}

	bool Window::ConsumeResize(unsigned& width, unsigned& height)
	{
		if (!m_ResizePending)
			return false;
		m_ResizePending = false;
		width  = m_PendingWidth;
		height = m_PendingHeight;
		return true;
	}

	LRESULT CALLBACK Window::WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window* self = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

		if (msg == WM_NCCREATE)
		{
			auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
			self = static_cast<Window*>(create->lpCreateParams);
			self->m_Handle = hwnd;
			::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}

		if (self)
			return self->WndProc(hwnd, msg, wParam, lParam);

		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	LRESULT Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// WM_SETCURSOR 需要区分命中区域：
		// 客户区交给 ImGui（决定 I 形光标、手形光标），非客户区交回系统（决定缩放箭头）。
		if (msg == WM_SETCURSOR && LOWORD(lParam) != HTCLIENT)
			return ::DefWindowProcW(hwnd, msg, wParam, lParam);

		if (m_MessageHook && m_MessageHook(hwnd, msg, wParam, lParam))
			return 1;

		switch (msg)
		{
		case WM_NCCALCSIZE:
			// 只有 wParam == TRUE 这一次才是「重新计算非客户区」的时机，
			// 返回 0 表示客户区覆盖整个窗口矩形。
			if (wParam == TRUE)
				return HandleNcCalcSize(lParam);
			break;

		case WM_NCHITTEST:
			return HandleNcHitTest(lParam);

		case WM_GETMINMAXINFO:
			return HandleGetMinMaxInfo(lParam);

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				m_PendingWidth  = LOWORD(lParam);
				m_PendingHeight = HIWORD(lParam);
				m_ResizePending = true;
			}
			return 0;

		case WM_DPICHANGED:
		{
			m_DpiScale = static_cast<float>(HIWORD(wParam)) / 96.0f;
			const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
			::SetWindowPos(hwnd, nullptr,
				suggested->left, suggested->top,
				suggested->right - suggested->left,
				suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			return 0;
		}

		case WM_ERASEBKGND:
			return 1;

		case WM_SYSCOMMAND:
			// 屏蔽 Alt 唤出系统菜单
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}

		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	LRESULT Window::HandleNcCalcSize(LPARAM lParam) const
	{
		auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

		// 最大化时把客户区收进显示器工作区。
		// 不做这一步的话，WS_THICKFRAME 会让最大化窗口溢出屏幕边缘几像素。
		if (::IsZoomed(m_Handle))
		{
			const HMONITOR monitor = ::MonitorFromWindow(m_Handle, MONITOR_DEFAULTTONEAREST);
			MONITORINFO info = { sizeof(MONITORINFO) };
			if (::GetMonitorInfoW(monitor, &info))
				params->rgrc[0] = info.rcWork;
		}

		return 0;   // 整个非客户区都不绘制
	}

	LRESULT Window::HandleNcHitTest(LPARAM lParam) const
	{
		POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		::ScreenToClient(m_Handle, &point);

		RECT client = {};
		::GetClientRect(m_Handle, &client);

		// 最大化状态下不允许拖动边框改尺寸，但标题栏仍交由系统处理，
		// 这样「拖动最大化窗口的标题栏可以还原」这一系统行为依旧有效。
		if (!::IsZoomed(m_Handle))
		{
			const LONG border = static_cast<LONG>(Chrome::ResizeBorder * m_DpiScale);
			const bool left   = point.x < border;
			const bool right  = point.x >= client.right - border;
			const bool top    = point.y < border;
			const bool bottom = point.y >= client.bottom - border;

			if (top && left)     return HTTOPLEFT;
			if (top && right)    return HTTOPRIGHT;
			if (bottom && left)  return HTBOTTOMLEFT;
			if (bottom && right) return HTBOTTOMRIGHT;
			if (left)   return HTLEFT;
			if (right)  return HTRIGHT;
			if (top)    return HTTOP;
			if (bottom) return HTBOTTOM;
		}

		// 标题栏左侧区域交给系统做拖动 / 双击最大化 / Aero Snap。
		// 右侧留给 ImGui，那里放语言切换和三个窗口按钮。
		const LONG titleHeight   = static_cast<LONG>(Chrome::TitleBarHeight * m_DpiScale);
		const LONG interactive   = static_cast<LONG>(Chrome::TitleInteractiveWidth * m_DpiScale);
		if (point.y < titleHeight && point.x < client.right - interactive)
			return HTCAPTION;

		return HTCLIENT;
	}

	LRESULT Window::HandleGetMinMaxInfo(LPARAM lParam) const
	{
		auto* info = reinterpret_cast<MINMAXINFO*>(lParam);

		// 让最大化后的窗口正好覆盖工作区，配合 HandleNcCalcSize 消除边缘溢出。
		const HMONITOR monitor = ::MonitorFromWindow(m_Handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
		if (::GetMonitorInfoW(monitor, &monitorInfo))
		{
			info->ptMaxPosition.x = monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
			info->ptMaxPosition.y = monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;
			info->ptMaxSize.x     = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
			info->ptMaxSize.y     = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
		}

		info->ptMinTrackSize.x = static_cast<LONG>(Chrome::MinWindowWidth * m_DpiScale);
		info->ptMinTrackSize.y = static_cast<LONG>(Chrome::MinWindowHeight * m_DpiScale);
		return 0;
	}
}
