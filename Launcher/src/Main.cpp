#include "Actions.h"
#include "Platform/D3D11Context.h"
#include "Platform/Paths.h"
#include "Platform/Texture.h"
#include "Platform/Window.h"
#include "Platform/WindowChrome.h"
#include "UI/LauncherState.h"
#include "UI/LauncherUI.h"
#include "UI/TextCoverage.h"
#include "UI/Theme.h"

#include <Windows.h>

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

// ImGui 的 Win32 后端把消息处理函数声明在 #if 0 里，要求使用方自行前向声明，
// 以免这个头文件反向依赖 <windows.h>。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	// 转发给 ImGui 的 Win32 后端。ImGui 只处理鼠标 / 键盘 / 光标这类消息，
	// WM_NCCALCSIZE、WM_NCHITTEST 等仍由 Window 自己处理。
	bool ForwardToImGui(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam) != 0;
	}

	// 启动器是 GUI 程序，但工程类型是 ConsoleApp（方便看日志）。
	// 默认隐藏控制台窗口，需要时用 --console 保留。
	void HideConsoleUnlessRequested(int argc, char** argv)
	{
		for (int index = 1; index < argc; ++index)
		{
			if (std::string(argv[index]) == "--console")
				return;
		}

		if (HWND console = ::GetConsoleWindow())
			::ShowWindow(console, SW_HIDE);
	}
}

int main(int argc, char** argv)
{
	using namespace Dark::Launcher;

	HideConsoleUnlessRequested(argc, argv);

	// 启动器是 GUI 程序，出错时没有控制台可看。
	// --log <path> 把启动日志同时落盘，作为排查线索。
	for (int index = 1; index < argc; ++index)
	{
		if (std::string(argv[index]) == "--log" && index + 1 < argc)
			Actions::OpenLogFile(argv[index + 1]);
	}

	// 让进程感知 DPI：高分屏下界面才不会被系统拉伸模糊。
	ImGui_ImplWin32_EnableDpiAwareness();

	Window window;
	if (!window.Create(L"Dark Launch", Chrome::DefaultWindowWidth, Chrome::DefaultWindowHeight))
		return 1;
	window.SetMessageHook(&ForwardToImGui);

	D3D11Context renderer;
	if (!renderer.Create(window.Handle()))
	{
		window.Destroy();
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr;    // 不落地布局缓存
	io.LogFilename = nullptr;

	Theme::ApplyStyle(window.DpiScale());

	ImGui_ImplWin32_Init(window.Handle());
	ImGui_ImplDX11_Init(renderer.Device(), renderer.DeviceContext());

	// ---- 状态初始化（当前全部来自假数据）----
	LauncherState state;
	Actions::RefreshModules(state);
	Actions::RefreshProjects(state);

	// --backend <opengl|vulkan|dx11> 预设渲染后端。
	// 启动器应当可脚本化：快捷方式、CI 冒烟测试都用得上。
	for (int index = 1; index < argc; ++index)
	{
		if (std::string(argv[index]) != "--backend" || index + 1 >= argc)
			continue;

		const std::string value = argv[index + 1];
		if (value == "opengl")
			state.Backend = BackendType::OpenGL;
		else if (value == "vulkan")
			state.Backend = BackendType::Vulkan;
		else if (value == "dx11")
			state.Backend = BackendType::DirectX11;
	}

	Actions::AppendLog(state, LogLevel::Info, "启动器界面已就绪", "Launcher UI is ready");

	// 品牌图标。属于可选资源：缺失时界面回退到内置字形，不影响运行。
	Texture logoTexture;
	{
		const std::filesystem::path logoPath = Paths::ResolveAsset("Content/Textures/DarkLogo.png");
		if (LoadTextureFromPng(renderer.Device(), logoPath, logoTexture))
		{
			Theme::SetLogo(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(logoTexture.View)),
				static_cast<float>(logoTexture.Width), static_cast<float>(logoTexture.Height));
			Actions::AppendLog(state, LogLevel::Debug,
				"已加载品牌图标: " + logoPath.filename().string() +
					" (" + std::to_string(logoTexture.Width) + "x" + std::to_string(logoTexture.Height) + ")",
				"Brand logo loaded: " + logoPath.filename().string() +
					" (" + std::to_string(logoTexture.Width) + "x" + std::to_string(logoTexture.Height) + ")");
		}
		else
		{
			Actions::AppendLog(state, LogLevel::Warning,
				"未找到品牌图标 Content/Textures/DarkLogo.png，已回退到内置字形",
				"Brand logo Content/Textures/DarkLogo.png not found, falling back to the built-in glyph");
		}
	}

	// 字形范围按「界面上真正出现的文字」构建，所以必须先把状态准备好，	// 再加载字体。这样界面上的每个字都保证有字形。
	std::vector<std::string> uiText;
	CollectUiText(state, uiText);
	const Theme::FontReport fonts = Theme::LoadFonts(window.DpiScale(), uiText);

	{
		const std::filesystem::path uiFont = Paths::ResolveUiFont();
		const std::filesystem::path iconFont = Paths::ResolveIconFont();
		const std::filesystem::path cjkFont = Paths::ResolveCjkFont();

		const std::string paths =
			uiFont.filename().string() + " + " +
			iconFont.filename().string() + " + " +
			cjkFont.filename().string();

		Actions::AppendLog(state, LogLevel::Debug, "字体来源: " + paths, "Font sources: " + paths);
		Actions::AppendLog(state, LogLevel::Debug,
			"DPI 缩放: " + std::to_string(window.DpiScale()) +
				"，纳入字形范围的文本 " + std::to_string(uiText.size()) + " 条",
			"DPI scale: " + std::to_string(window.DpiScale()) +
				", " + std::to_string(uiText.size()) + " text literals added to the glyph range");
	}

	// 字形自检结果写进日志和标准输出：字体缺字形时界面上只会看到方框，
	// 很难反查，所以启动时就把覆盖情况明确报出来。
	std::printf("[Dark-Launcher] dpi=%.3f uiFont=%d iconFont=%d cjkFont=%d iconGlyph=%d cjkGlyph=%d\n",
		window.DpiScale(),
		fonts.UiFontLoaded ? 1 : 0,
		fonts.IconFontLoaded ? 1 : 0,
		fonts.CjkFontLoaded ? 1 : 0,
		fonts.IconGlyphAvailable ? 1 : 0,
		fonts.CjkGlyphAvailable ? 1 : 0);
	std::fflush(stdout);

	if (!fonts.Complete())
	{
		Actions::AppendLog(state, LogLevel::Warning,
			"字体覆盖不完整，界面上的图标或中文可能显示为方框",
			"Font coverage is incomplete; icons or Chinese text may render as boxes");
	}

	UI::HostCommands host;
	host.Minimize = [&window]() { window.Minimize(); };
	host.ToggleMaximize = [&window]() { window.ToggleMaximize(); };
	host.Close = [&window]() { window.RequestClose(); };

	const float clearColor[4] = { 0.051f, 0.059f, 0.078f, 1.0f };

	bool running = true;
	bool presentFailureReported = false;
	while (running)
	{
		MSG message;
		while (::PeekMessageW(&message, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessageW(&message);
			if (message.message == WM_QUIT)
				running = false;
		}
		if (!running)
			break;

		unsigned resizeWidth = 0;
		unsigned resizeHeight = 0;
		if (window.ConsumeResize(resizeWidth, resizeHeight))
			renderer.Resize(resizeWidth, resizeHeight);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		const float deltaTime = (io.DeltaTime > 0.0f && io.DeltaTime < 0.5f)
			? io.DeltaTime
			: 1.0f / 60.0f;

		host.Maximized = window.IsWindowMaximized();

		Actions::Tick(state, deltaTime);
		UI::Draw(state, host, deltaTime);

		ImGui::Render();
		renderer.BeginFrame(clearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		renderer.Present(state.Settings.VSync);

		// Present 失败时画面会停在最后一帧，看不出任何异常，
		// 所以第一次失败就要明确记一笔。
		if (FAILED(renderer.LastPresentResult()) && !presentFailureReported)
		{
			presentFailureReported = true;
			char detail[96];
			std::snprintf(detail, sizeof(detail), "0x%08lX",
				static_cast<unsigned long>(renderer.LastPresentResult()));
			Actions::AppendLog(state, LogLevel::Warning,
				std::string("交换链 Present 失败，画面将停止刷新，HRESULT=") + detail,
				std::string("Swap chain Present failed, the window will stop updating. HRESULT=") + detail);
		}
	}

	Actions::CloseLogFile();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	ReleaseTexture(logoTexture);
	renderer.Destroy();
	window.Destroy();
	return 0;
}
