#pragma once

#include "UI/LauncherState.h"

namespace Dark::Launcher::MockData
{
	// 界面阶段用的假数据。
	//
	// 这些内容刻意照着仓库里真实存在的组件写（Dark-Render / Dark-Resources /
	// Dark-Tools / Dark-Script 的版本号与 DLL 名都对得上），
	// 这样界面看起来就是最终形态，接线时只要把来源换成真实扫描结果。
	void PopulateModules(LauncherState& state);
	void PopulateProjects(LauncherState& state);
}
