#pragma once

// 窗口外壳的尺寸常量。
// 全部以 100% DPI 的逻辑像素书写，运行时乘以 Window::DpiScale() 得到物理像素。
namespace Dark::Launcher::Chrome
{
	constexpr float TitleBarHeight    = 40.0f;
	constexpr float TitleButtonWidth  = 46.0f;
	constexpr float LanguagePillWidth = 84.0f;
	constexpr float TitleRightPadding = 10.0f;

	// 标题栏右侧需要留给 ImGui 接收点击的区域宽度（语言切换 + 最小化/最大化/关闭）。
	// WM_NCHITTEST 把这部分当客户区处理，其余标题栏返回 HTCAPTION 交给系统拖动。
	constexpr float TitleInteractiveWidth =
		TitleButtonWidth * 3.0f + LanguagePillWidth + TitleRightPadding * 2.0f;

	constexpr float NavWidth        = 238.0f;
	constexpr float BottomBarHeight = 64.0f;
	constexpr float ResizeBorder    = 6.0f;
	constexpr float ContentPadding  = 26.0f;

	constexpr float MinWindowWidth      = 1000.0f;
	constexpr float MinWindowHeight     = 660.0f;
	constexpr float DefaultWindowWidth  = 1140.0f;
	constexpr float DefaultWindowHeight = 740.0f;
}
