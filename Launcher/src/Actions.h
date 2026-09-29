#pragma once

#include "UI/LauncherState.h"

#include <string>

namespace Dark::Launcher::Actions
{
	// ============================================================================
	// 界面与「真实功能」之间唯一的接口。
	//
	// 当前全部是假实现：只推进 LauncherState 里的状态，不加载任何 DLL、
	// 不启动任何进程。界面代码只依赖这一组函数，因此后续接入真实功能时
	// 只需要重写本文件，UI 一行都不用改。
	//
	// 将来的分工大致是：
	//   RefreshModules -> 扫描 bin/ 目录，读取每个模块的版本与 ABI 号
	//   RefreshProjects-> 读取最近项目列表（注册表或配置文件）
	//   Validate       -> 版本 / 依赖 / ABI 校验
	//   Launch         -> LoadLibrary 各模块，或 CreateProcess 拉起宿主进程
	//   Stop           -> 卸载模块 / 结束进程
	//   Tick           -> 轮询启动进度（真实实现里是查询子进程或加载阶段）
	// ============================================================================

	void RefreshModules(LauncherState& state);
	void RefreshProjects(LauncherState& state);

	// 返回当前装配是否满足启动条件。真实实现里这里做版本与 ABI 校验。
	bool Validate(LauncherState& state);

	void Launch(LauncherState& state);
	void Stop(LauncherState& state);

	// 主循环每帧调用，推进启动流程。
	void Tick(LauncherState& state, float deltaTime);

	// 追加一行启动日志。
	void AppendLog(LauncherState& state, LogLevel level,
		const std::string& chinese, const std::string& english);

	// 把日志同时写入文件（对应命令行 --log <path>）。
	// 启动器是 GUI 程序，出错时没有控制台可看，落盘日志是唯一的排查线索。
	void OpenLogFile(const std::string& path);
	void CloseLogFile();
}
