#pragma once

#include "UI/LauncherState.h"

#include <imgui.h>

// 自绘控件集合。全部以 ImDrawList 手工绘制，
// 尺寸参数已包含 DPI 缩放（调用方传入 Theme::Scale() 的结果）。
namespace Dark::Launcher::UI
{
	// ---- 标题栏 ----
	// 纯图标按钮，返回是否被点击。
	bool IconButton(const char* id, const char* glyph, ImVec2 size, ImVec4 hoverBg, ImVec4 glyphColor);

	// ---- 面板 ----
	// 带标题的圆角面板，内部自带内边距。必须与 EndPanel() 配对。
	// autoHeight 为真时高度完全由内容决定（用于纵向堆叠的页面，
	// 这样不必手写高度，也就不会把内容裁掉）；为假时使用传入的 size。
	void BeginPanel(const char* id, ImVec2 size, const char* title, const char* subtitle,
		float scale, bool autoHeight = false);
	// 边框在内容排布完成后才绘制，因此 AutoResizeY 也能描对。
	void EndPanel(float scale);

	// ---- 徽章 ----
	void Chip(const char* text, ImVec4 textColor, ImVec4 bgColor, float scale);
	void ChipIcon(const char* glyph, const char* text, ImVec4 textColor, ImVec4 bgColor, float scale);
	void StatusBadge(ModuleStatus status, Language language, float scale, bool dimmed);

	// ---- 按钮 ----
	bool PrimaryButton(const char* label, const char* glyph, ImVec2 size, float scale, bool enabled);
	bool GhostButton(const char* label, const char* glyph, ImVec2 size, float scale, bool enabled);

	// ---- 开关 ----
	bool ToggleSwitch(const char* id, bool value, float scale);

	// ---- 其他 ----
	void SectionLabel(const char* glyph, const char* text, float scale);
	void ProgressStrip(float progress, ImVec4 color, ImVec2 size, float scale);
	void Divider(float scale);
	void KeyValueRow(const char* key, const char* value, float scale);
}
