#pragma once

#include "UI/LauncherState.h"

#include <string>
#include <vector>

namespace Dark::Launcher
{
	// 收集界面上会用到的全部文字。
	//
	// 用途是构建字体图集的字形范围：ImGui 自带的
	// GetGlyphRangesChineseSimplifiedCommon() 只覆盖「常用 2500 字」，
	// 而界面上的「渲」「帧」「辨」这类字并不保证在表内，漏掉就会显示成方框。
	// 所以这里把字符串表和当前状态里的文案全部收集起来，逐个加入范围。
	void CollectUiText(const LauncherState& state, std::vector<std::string>& out);
}
