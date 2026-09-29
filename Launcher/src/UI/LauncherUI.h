#pragma once

#include "UI/LauncherState.h"

#include <functional>

namespace Dark::Launcher::UI
{
	// 界面需要回调宿主窗口的地方。用 std::function 而不是函数指针，
	// 是因为这里只服务于本进程的 UI，不跨越任何模块边界。
	struct HostCommands
	{
		std::function<void()> Minimize;
		std::function<void()> ToggleMaximize;
		std::function<void()> Close;
		bool Maximized = false;
	};

	// 绘制整个启动器界面（标题栏 + 导航 + 当前页面 + 底栏 + 弹窗）。
	void Draw(LauncherState& state, const HostCommands& host, float deltaTime);
}
