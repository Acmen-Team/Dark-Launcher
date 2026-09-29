#include "UI/Widgets.h"

#include "UI/Strings.h"
#include "UI/Theme.h"

#include <algorithm>
#include <string>

namespace Dark::Launcher::UI
{
	namespace
	{
		ImVec4 Brighten(const ImVec4& color, float amount)
		{
			return ImVec4(
				std::min(1.0f, color.x + amount),
				std::min(1.0f, color.y + amount),
				std::min(1.0f, color.z + amount),
				color.w);
		}

		ImVec4 WithAlpha(const ImVec4& color, float alpha)
		{
			return ImVec4(color.x, color.y, color.z, alpha);
		}
	}

	// ---------------------------------------------------------------- 标题栏

	bool IconButton(const char* id, const char* glyph, ImVec2 size, ImVec4 hoverBg, ImVec4 glyphColor)
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton(id, size);

		const bool hovered = ImGui::IsItemHovered();
		const bool clicked = ImGui::IsItemClicked();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

		if (hovered)
			draw->AddRectFilled(pos, max, ImGui::GetColorU32(hoverBg));

		const ImVec2 textSize = ImGui::CalcTextSize(glyph);
		const ImVec2 textPos = ImVec2(
			pos.x + (size.x - textSize.x) * 0.5f,
			pos.y + (size.y - textSize.y) * 0.5f);
		draw->AddText(textPos, ImGui::GetColorU32(glyphColor), glyph);

		return clicked;
	}

	// ---------------------------------------------------------------- 面板

	void BeginPanel(const char* id, ImVec2 size, const char* title, const char* subtitle,
		float scale, bool autoHeight)
	{
		const Theme::Palette& colors = Theme::Colors();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.Surface);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f * scale, 18.0f * scale));
		ImGui::BeginChild(id, size,
			ImGuiChildFlags_AlwaysUseWindowPadding |
				(autoHeight ? ImGuiChildFlags_AutoResizeY : ImGuiChildFlags_None),
			ImGuiWindowFlags_None);
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		if (title && *title)
		{
			if (ImFont* font = Theme::Fonts().Title)
				ImGui::PushFont(font);
			ImGui::TextColored(colors.TextPrimary, "%s", title);
			if (Theme::Fonts().Title)
				ImGui::PopFont();

			if (subtitle && *subtitle)
			{
				if (ImFont* small = Theme::Fonts().Small)
					ImGui::PushFont(small);
				ImGui::TextColored(colors.TextMuted, "%s", subtitle);
				if (Theme::Fonts().Small)
					ImGui::PopFont();
			}

			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
		}
	}

	void EndPanel(float scale)
	{
		// 边框必须等到内容排布完成后再画：AutoResizeY 的面板只有在
		// 内容都提交之后，GetWindowSize() 才是最终高度。
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 min = ImGui::GetWindowPos();
		const ImVec2 size = ImGui::GetWindowSize();
		draw->AddRect(min, ImVec2(min.x + size.x, min.y + size.y),
			ImGui::GetColorU32(Theme::Colors().Border), 10.0f * scale, 0, 1.0f);

		ImGui::EndChild();
	}

	// ---------------------------------------------------------------- 徽章

	void Chip(const char* text, ImVec4 textColor, ImVec4 bgColor, float scale)
	{
		const ImVec2 padding = ImVec2(10.0f * scale, 4.0f * scale);
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 size = ImVec2(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f);

		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);
		const float rounding = size.y * 0.5f;

		draw->AddRectFilled(pos, max, ImGui::GetColorU32(bgColor), rounding);
		draw->AddRect(pos, max, ImGui::GetColorU32(WithAlpha(textColor, textColor.w * 0.3f)), rounding, 0, 1.0f);
		draw->AddText(ImVec2(pos.x + padding.x, pos.y + padding.y), ImGui::GetColorU32(textColor), text);

		ImGui::Dummy(size);
	}

	void ChipIcon(const char* glyph, const char* text, ImVec4 textColor, ImVec4 bgColor, float scale)
	{
		Chip(glyph && *glyph ? (std::string(glyph) + "  " + text).c_str() : text, textColor, bgColor, scale);
	}

	void StatusBadge(ModuleStatus status, Language language, float scale, bool dimmed)
	{
		const Theme::Palette& colors = Theme::Colors();

		S key = S::StatusReady;
		ImVec4 color = colors.Success;
		ImVec4 background = colors.SuccessSoft;

		switch (status)
		{
		case ModuleStatus::Ready:
			break;

		case ModuleStatus::Missing:
			key = S::StatusMissing;
			color = colors.Danger;
			background = colors.DangerSoft;
			break;

		case ModuleStatus::Incompatible:
			key = S::StatusIncompatible;
			color = colors.Warning;
			background = colors.WarningSoft;
			break;

		case ModuleStatus::Disabled:
			key = S::StatusDisabled;
			color = colors.TextMuted;
			background = colors.SurfaceAlt;
			break;
		}

		if (dimmed)
		{
			color = WithAlpha(color, color.w * 0.5f);
			background = WithAlpha(background, background.w * 0.5f);
		}

		if (ImFont* small = Theme::Fonts().Small)
			ImGui::PushFont(small);
		Chip(Tr(key, language), color, background, scale);
		if (Theme::Fonts().Small)
			ImGui::PopFont();
	}

	// ---------------------------------------------------------------- 按钮

	bool PrimaryButton(const char* label, const char* glyph, ImVec2 size, float scale, bool enabled)
	{
		const Theme::Palette& colors = Theme::Colors();

		ImGui::PushStyleColor(ImGuiCol_Button, colors.Accent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.AccentHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.Accent);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f * scale);

		const std::string text = (glyph && *glyph)
			? std::string(glyph) + "   " + label
			: std::string(label);

		if (!enabled)
			ImGui::BeginDisabled();
		const bool clicked = ImGui::Button(text.c_str(), size);
		if (!enabled)
			ImGui::EndDisabled();

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);

		return clicked && enabled;
	}

	bool GhostButton(const char* label, const char* glyph, ImVec2 size, float scale, bool enabled)
	{
		const Theme::Palette& colors = Theme::Colors();

		ImGui::PushStyleColor(ImGuiCol_Button, colors.SurfaceAlt);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.SurfaceHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.SurfaceActive);
		ImGui::PushStyleColor(ImGuiCol_Text, colors.TextPrimary);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f * scale);

		const std::string text = (glyph && *glyph)
			? std::string(glyph) + "   " + label
			: std::string(label);

		if (!enabled)
			ImGui::BeginDisabled();
		const bool clicked = ImGui::Button(text.c_str(), size);
		if (!enabled)
			ImGui::EndDisabled();

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);

		return clicked && enabled;
	}

	// ---------------------------------------------------------------- 开关

	bool ToggleSwitch(const char* id, bool value, float scale)
	{
		const Theme::Palette& colors = Theme::Colors();

		const ImVec2 size = ImVec2(42.0f * scale, 23.0f * scale);
		const ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton(id, size);
		const bool clicked = ImGui::IsItemClicked();
		const bool hovered = ImGui::IsItemHovered();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);
		const float radius = size.y * 0.5f;

		ImVec4 track = value ? colors.Accent : colors.SurfaceActive;
		if (hovered)
			track = Brighten(track, 0.07f);

		draw->AddRectFilled(pos, max, ImGui::GetColorU32(track), radius);

		const float knobRadius = radius - 3.0f * scale;
		const float knobCenterX = value ? (max.x - radius) : (pos.x + radius);
		draw->AddCircleFilled(ImVec2(knobCenterX, pos.y + radius), knobRadius,
			ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.95f)), 24);

		return clicked;
	}

	// ---------------------------------------------------------------- 其他

	void SectionLabel(const char* glyph, const char* text, float scale)
	{
		(void)scale;
		if (ImFont* small = Theme::Fonts().Small)
			ImGui::PushFont(small);
		ImGui::TextColored(Theme::Colors().TextMuted, "%s   %s", glyph ? glyph : "", text);
		if (Theme::Fonts().Small)
			ImGui::PopFont();
	}

	void ProgressStrip(float progress, ImVec4 color, ImVec2 size, float scale)
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);
		const float rounding = size.y * 0.5f;

		draw->AddRectFilled(pos, max, ImGui::GetColorU32(Theme::Colors().SurfaceActive), rounding);

		const float clamped = std::max(0.0f, std::min(1.0f, progress));
		const float filled = clamped * size.x;
		if (filled > 1.0f)
		{
			draw->AddRectFilled(pos, ImVec2(pos.x + filled, max.y), ImGui::GetColorU32(color), rounding);
		}

		(void)scale;
		ImGui::Dummy(size);
	}

	void Divider(float scale)
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const float width = ImGui::GetContentRegionAvail().x;
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddLine(
			ImVec2(pos.x, pos.y + scale),
			ImVec2(pos.x + width, pos.y + scale),
			ImGui::GetColorU32(Theme::Colors().BorderSoft));
		ImGui::Dummy(ImVec2(width, 2.0f * scale));
	}

	void KeyValueRow(const char* key, const char* value, float scale)
	{
		(void)scale;
		const Theme::Palette& colors = Theme::Colors();

		if (ImFont* small = Theme::Fonts().Small)
			ImGui::PushFont(small);
		ImGui::TextColored(colors.TextMuted, "%s", key);
		if (Theme::Fonts().Small)
			ImGui::PopFont();

		ImGui::SameLine();

		const float valueWidth = ImGui::CalcTextSize(value).x;
		const float available = ImGui::GetContentRegionAvail().x;
		if (available > valueWidth)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - valueWidth);

		ImGui::TextColored(colors.TextPrimary, "%s", value);
	}
}
