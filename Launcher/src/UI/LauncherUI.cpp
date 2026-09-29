#include "UI/LauncherUI.h"

#include "Actions.h"
#include "Platform/WindowChrome.h"
#include "UI/Icons.h"
#include "UI/Strings.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <string>

namespace Dark::Launcher::UI
{
	namespace
	{
		// ------------------------------------------------------------ 绘制小工具

		ImVec4 Hex(unsigned int rgb, float alpha = 1.0f)
		{
			return ImVec4(
				static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f,
				static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
				static_cast<float>(rgb & 0xFFu) / 255.0f,
				alpha);
		}

		ImFont* Resolve(ImFont* font)
		{
			return font ? font : ImGui::GetFont();
		}

		ImVec2 TextSize(ImFont* font, const char* text)
		{
			ImFont* resolved = Resolve(font);
			return resolved->CalcTextSizeA(resolved->FontSize, FLT_MAX, 0.0f, text);
		}

		void Text(ImDrawList* draw, ImFont* font, ImVec2 position, ImVec4 color, const char* text)
		{
			ImFont* resolved = Resolve(font);
			draw->AddText(resolved, resolved->FontSize, position, ImGui::GetColorU32(color), text);
		}

		// 在给定宽高的方框内绘制文字，可选择垂直 / 水平居中。
		void TextBox(ImDrawList* draw, ImFont* font, ImVec2 boxMin, float boxWidth, float boxHeight,
			ImVec4 color, const char* text, bool centerHorizontally = false)
		{
			const ImVec2 size = TextSize(font, text);
			const float x = centerHorizontally ? boxMin.x + (boxWidth - size.x) * 0.5f : boxMin.x;
			Text(draw, font, ImVec2(x, boxMin.y + (boxHeight - size.y) * 0.5f), color, text);
		}

		ImVec4 WithAlpha(const ImVec4& color, float alpha)
		{
			return ImVec4(color.x, color.y, color.z, alpha);
		}

		// 把品牌图标等比缩放后绘制在正方形框内（居中）。
		// 返回 false 表示图标不可用，调用方应回退到内置字形。
		bool DrawLogoInBox(ImDrawList* draw, ImVec2 boxMin, float boxSize, float inset)
		{
			const Theme::LogoImage& logo = Theme::Logo();
			const float available = boxSize - inset * 2.0f;
			if (!logo.Valid() || available <= 0.0f)
				return false;

			// 标记本身是竖向的，等比缩放后宽度自然小于框宽。
			const float aspect = logo.Width / logo.Height;
			float width = available;
			float height = available;
			if (aspect > 1.0f)
				height = available / aspect;
			else
				width = available * aspect;

			const ImVec2 min = ImVec2(
				boxMin.x + (boxSize - width) * 0.5f,
				boxMin.y + (boxSize - height) * 0.5f);
			draw->AddImage(logo.Texture, min, ImVec2(min.x + width, min.y + height));
			return true;
		}

		// 徽章总宽（与 Widgets::Chip 的尺寸保持一致）。
		float ChipWidth(const char* text, float scale)
		{
			return TextSize(Theme::Fonts().Small, text).x + 20.0f * scale;
		}

		S StatusKey(ModuleStatus status)
		{
			switch (status)
			{
			case ModuleStatus::Missing:      return S::StatusMissing;
			case ModuleStatus::Incompatible: return S::StatusIncompatible;
			case ModuleStatus::Disabled:     return S::StatusDisabled;
			default:                         return S::StatusReady;
			}
		}

		ImVec4 StatusColor(ModuleStatus status)
		{
			const Theme::Palette& c = Theme::Colors();
			switch (status)
			{
			case ModuleStatus::Missing:      return c.Danger;
			case ModuleStatus::Incompatible: return c.Warning;
			case ModuleStatus::Disabled:     return c.TextMuted;
			default:                         return c.Success;
			}
		}

		const char* BackendLabel(BackendType backend)
		{
			switch (backend)
			{
			case BackendType::OpenGL:    return Tr(S::BackendOpenGLName, Language::English);
			case BackendType::Vulkan:    return Tr(S::BackendVulkanName, Language::English);
			case BackendType::DirectX11: return Tr(S::BackendDX11Name, Language::English);
			default:                     return "";
			}
		}

		const char* LogLevelLabel(LogLevel level, Language language)
		{
			switch (level)
			{
			case LogLevel::Trace:   return Tr(S::LogTrace, language);
			case LogLevel::Debug:   return Tr(S::LogDebug, language);
			case LogLevel::Warning: return Tr(S::LogWarning, language);
			case LogLevel::Error:   return Tr(S::LogError, language);
			default:                return Tr(S::LogInfo, language);
			}
		}

		ImVec4 LogLevelColor(LogLevel level)
		{
			const Theme::Palette& c = Theme::Colors();
			switch (level)
			{
			case LogLevel::Trace:   return c.TextMuted;
			case LogLevel::Debug:   return c.TextSecondary;
			case LogLevel::Warning: return c.Warning;
			case LogLevel::Error:   return c.Danger;
			default:                return c.TextPrimary;
			}
		}

		const char* PhaseLabel(LaunchPhase phase, Language language)
		{
			switch (phase)
			{
			case LaunchPhase::Validating: return Tr(S::PhaseValidating, language);
			case LaunchPhase::Launching:  return Tr(S::PhaseLaunching, language);
			case LaunchPhase::Running:    return Tr(S::PhaseRunning, language);
			case LaunchPhase::Failed:     return Tr(S::PhaseFailed, language);
			default:                      return Tr(S::PhaseIdle, language);
			}
		}

		ImVec4 PhaseColor(LaunchPhase phase)
		{
			const Theme::Palette& c = Theme::Colors();
			switch (phase)
			{
			case LaunchPhase::Validating:
			case LaunchPhase::Launching: return c.Info;
			case LaunchPhase::Running:   return c.Success;
			case LaunchPhase::Failed:    return c.Danger;
			default:                     return c.TextMuted;
			}
		}

		std::string FormatClock(float seconds)
		{
			const int total = static_cast<int>(seconds);
			char buffer[16];
			std::snprintf(buffer, sizeof(buffer), "%02d:%02d", total / 60, total % 60);
			return buffer;
		}

		std::string JoinDependencies(const ModuleEntry& module)
		{
			std::string result;
			for (std::size_t index = 0; index < module.DependsOn.size(); ++index)
			{
				if (index > 0)
					result += " · ";
				result += module.DependsOn[index];
			}
			return result;
		}

		void PageHeader(const char* title, const char* subtitle, float scale)
		{
			const Theme::Palette& c = Theme::Colors();

			if (ImFont* font = Theme::Fonts().Title)
				ImGui::PushFont(font);
			ImGui::TextColored(c.TextPrimary, "%s", title);
			if (Theme::Fonts().Title)
				ImGui::PopFont();

			if (ImFont* font = Theme::Fonts().Small)
				ImGui::PushFont(font);
			ImGui::TextColored(c.TextMuted, "%s", subtitle);
			if (Theme::Fonts().Small)
				ImGui::PopFont();

			ImGui::Dummy(ImVec2(0.0f, 12.0f * scale));
		}

		// 可点击的小胶囊，绘制后把光标向右移动，便于同行继续排。
		bool SelectableChip(const char* id, const char* label, bool selected, float scale)
		{
			const Theme::Palette& c = Theme::Colors();
			const float height = 30.0f * scale;
			const float padding = 13.0f * scale;
			const ImVec2 textSize = TextSize(Theme::Fonts().Small, label);
			const ImVec2 size = ImVec2(textSize.x + padding * 2.0f, height);
			const ImVec2 position = ImGui::GetCursorScreenPos();

			ImGui::InvisibleButton(id, size);
			const bool hovered = ImGui::IsItemHovered();
			const bool clicked = ImGui::IsItemClicked();

			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImVec2 max = ImVec2(position.x + size.x, position.y + size.y);
			const float rounding = height * 0.5f;

			const ImVec4 background = selected ? c.AccentSoft : (hovered ? c.SurfaceHover : c.SurfaceAlt);
			const ImVec4 border = selected ? WithAlpha(c.Accent, 0.55f) : c.Border;

			draw->AddRectFilled(position, max, ImGui::GetColorU32(background), rounding);
			draw->AddRect(position, max, ImGui::GetColorU32(border), rounding, 0, 1.0f);
			TextBox(draw, Theme::Fonts().Small, ImVec2(position.x + padding, position.y),
				textSize.x, height, selected ? c.Accent : c.TextSecondary, label);

			ImGui::SetCursorScreenPos(ImVec2(max.x + 8.0f * scale, position.y));
			return clicked;
		}

		// 纯信息胶囊：不可点击，用于展示一组标签（例如依赖列表）。
		// 与 SelectableChip 的区别是不给用户「可以点」的错觉。绘制后光标右移。
		void InfoChip(const char* text, float scale)
		{
			const Theme::Palette& c = Theme::Colors();
			const float height = 28.0f * scale;
			const float padding = 12.0f * scale;
			const ImVec2 textSize = TextSize(Theme::Fonts().Small, text);
			const ImVec2 size = ImVec2(textSize.x + padding * 2.0f, height);
			const ImVec2 position = ImGui::GetCursorScreenPos();

			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImVec2 max = ImVec2(position.x + size.x, position.y + size.y);
			const float rounding = height * 0.5f;

			draw->AddRectFilled(position, max, ImGui::GetColorU32(c.SurfaceAlt), rounding);
			draw->AddRect(position, max, ImGui::GetColorU32(c.Border), rounding, 0, 1.0f);
			TextBox(draw, Theme::Fonts().Small, ImVec2(position.x + padding, position.y),
				textSize.x, height, c.TextSecondary, text);

			ImGui::SetCursorScreenPos(ImVec2(max.x + 8.0f * scale, position.y));
		}

		// 标签在左、开关在右的一行。
		bool ToggleRow(const char* id, const char* label, bool value, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();
			const float rowHeight = 34.0f * scale;
			const float toggleWidth = 42.0f * scale;
			const ImVec2 position = ImGui::GetCursorScreenPos();

			ImGui::PushID(id);
			ImGui::SetCursorScreenPos(ImVec2(
				position.x + width - toggleWidth,
				position.y + (rowHeight - 23.0f * scale) * 0.5f));
			const bool clicked = ToggleSwitch("##toggle", value, scale);
			ImGui::PopID();

			const ImVec2 labelSize = TextSize(Theme::Fonts().Body, label);
			Text(ImGui::GetWindowDrawList(), Theme::Fonts().Body,
				ImVec2(position.x, position.y + (rowHeight - labelSize.y) * 0.5f),
				c.TextPrimary, label);

			ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + rowHeight));
			return clicked;
		}

		// ------------------------------------------------------------ 标题栏

		void DrawTitleBar(LauncherState& state, const HostCommands& host, ImVec2 display, float scale)
		{
			const Theme::Palette& c = Theme::Colors();
			const float height = Chrome::TitleBarHeight * scale;
			const float pillWidth = Chrome::LanguagePillWidth * scale;
			const float buttonWidth = Chrome::TitleButtonWidth * scale;
			const float rightPadding = Chrome::TitleRightPadding * scale;

			ImDrawList* draw = ImGui::GetWindowDrawList();
			draw->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(display.x, height), ImGui::GetColorU32(c.NavBg));
			draw->AddLine(ImVec2(0.0f, height), ImVec2(display.x, height), ImGui::GetColorU32(c.BorderSoft));

			// 品牌图标 + 名称 + 版本
			const float logoSize = 22.0f * scale;
			const float iconX = 16.0f * scale;
			if (!DrawLogoInBox(draw, ImVec2(iconX, (height - logoSize) * 0.5f), logoSize, 0.0f))
			{
				// 图标缺失时回退到内置字形
				TextBox(draw, Theme::Fonts().Body, ImVec2(iconX, 0.0f), 0.0f, height, c.Accent, Icons::Cube);
			}

			const char* appTitle = Tr(S::AppTitle, state.Lang);
			const float nameX = iconX + logoSize + 9.0f * scale;
			TextBox(draw, Theme::Fonts().Body, ImVec2(nameX, 0.0f), 0.0f, height, c.TextPrimary, appTitle);

			const float versionX = nameX + TextSize(Theme::Fonts().Body, appTitle).x + 12.0f * scale;
			TextBox(draw, Theme::Fonts().Small, ImVec2(versionX, 0.0f), 0.0f, height, c.TextMuted,
				Tr(S::VersionLabel, state.Lang));

			// 分隔点 + 标语
			const float taglineX = versionX + TextSize(Theme::Fonts().Small, Tr(S::VersionLabel, state.Lang)).x + 14.0f * scale;
			TextBox(draw, Theme::Fonts().Small, ImVec2(taglineX, 0.0f), 0.0f, height, c.TextMuted, "·");
			TextBox(draw, Theme::Fonts().Small, ImVec2(taglineX + 12.0f * scale, 0.0f), 0.0f, height, c.TextMuted,
				Tr(S::AppTagline, state.Lang));

			// ---- 右侧：语言切换 + 窗口按钮 ----
			const float buttonsX = display.x - buttonWidth * 3.0f;
			const float pillX = buttonsX - rightPadding - pillWidth;

			{
				const float pillHeight = 26.0f * scale;
				const ImVec2 pillMin = ImVec2(pillX, (height - pillHeight) * 0.5f);
				const ImVec2 pillMax = ImVec2(pillX + pillWidth, pillMin.y + pillHeight);
				const float rounding = pillHeight * 0.5f;
				const float half = pillWidth * 0.5f;
				const bool chinese = (state.Lang == Language::Chinese);

				draw->AddRectFilled(pillMin, pillMax, ImGui::GetColorU32(c.SurfaceAlt), rounding);
				draw->AddRect(pillMin, pillMax, ImGui::GetColorU32(c.Border), rounding, 0, 1.0f);

				if (chinese)
					draw->AddRectFilled(pillMin, ImVec2(pillMin.x + half, pillMax.y), ImGui::GetColorU32(c.Accent), rounding);
				else
					draw->AddRectFilled(ImVec2(pillMin.x + half, pillMin.y), pillMax, ImGui::GetColorU32(c.Accent), rounding);

				const ImVec4 onColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				TextBox(draw, Theme::Fonts().Small, ImVec2(pillMin.x, pillMin.y), half, pillHeight,
					chinese ? onColor : c.TextSecondary, Tr(S::LangChineseShort, state.Lang), true);
				TextBox(draw, Theme::Fonts().Small, ImVec2(pillMin.x + half, pillMin.y), half, pillHeight,
					chinese ? c.TextSecondary : onColor, Tr(S::LangEnglishShort, state.Lang), true);

				ImGui::SetCursorScreenPos(pillMin);
				ImGui::InvisibleButton("##lang-zh", ImVec2(half, pillHeight));
				if (ImGui::IsItemClicked())
					state.Lang = Language::Chinese;

				ImGui::SetCursorScreenPos(ImVec2(pillMin.x + half, pillMin.y));
				ImGui::InvisibleButton("##lang-en", ImVec2(half, pillHeight));
				if (ImGui::IsItemClicked())
					state.Lang = Language::English;
			}

			const ImVec2 buttonSize = ImVec2(buttonWidth, height);

			ImGui::SetCursorScreenPos(ImVec2(buttonsX, 0.0f));
			if (IconButton("##minimize", Icons::Minus, buttonSize, c.SurfaceHover, c.TextSecondary))
			{
				if (host.Minimize)
					host.Minimize();
			}

			ImGui::SetCursorScreenPos(ImVec2(buttonsX + buttonWidth, 0.0f));
			if (IconButton("##maximize", host.Maximized ? Icons::Compress : Icons::Expand, buttonSize,
				c.SurfaceHover, c.TextSecondary))
			{
				if (host.ToggleMaximize)
					host.ToggleMaximize();
			}

			ImGui::SetCursorScreenPos(ImVec2(buttonsX + buttonWidth * 2.0f, 0.0f));
			if (IconButton("##close", Icons::Times, buttonSize, c.Danger, c.TextSecondary))
			{
				if (host.Close)
					host.Close();
			}
		}

		// ------------------------------------------------------------ 左侧导航

		void DrawNavItem(LauncherState& state, int index, const char* name, const char* hint, float scale)
		{
			const Theme::Palette& c = Theme::Colors();

			const float itemHeight = 54.0f * scale;
			const float width = ImGui::GetContentRegionAvail().x;
			const ImVec2 position = ImGui::GetCursorScreenPos();
			const bool selected = (static_cast<int>(state.Current) == index);
			const bool completed = (static_cast<int>(state.Current) > index);

			ImGui::PushID(index);
			ImGui::InvisibleButton("##nav-item", ImVec2(width, itemHeight));
			const bool hovered = ImGui::IsItemHovered();
			const bool clicked = ImGui::IsItemClicked();
			ImGui::PopID();

			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImVec2 max = ImVec2(position.x + width, position.y + itemHeight);
			const float rounding = 9.0f * scale;

			if (selected)
				draw->AddRectFilled(position, max, ImGui::GetColorU32(c.AccentSoft), rounding);
			else if (hovered)
				draw->AddRectFilled(position, max, ImGui::GetColorU32(c.SurfaceHover), rounding);

			if (selected)
				draw->AddRectFilled(position, ImVec2(position.x + 3.0f * scale, max.y),
					ImGui::GetColorU32(c.Accent), rounding);

			const float badge = 26.0f * scale;
			const ImVec2 badgeMin = ImVec2(position.x + 16.0f * scale, position.y + (itemHeight - badge) * 0.5f);
			const ImVec2 badgeMax = ImVec2(badgeMin.x + badge, badgeMin.y + badge);

			ImVec4 badgeColor = c.SurfaceAlt;
			if (selected)
				badgeColor = c.Accent;
			else if (completed)
				badgeColor = c.SuccessSoft;
			draw->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(badgeColor), badge * 0.5f);

			if (completed)
			{
				TextBox(draw, Theme::Fonts().Small, badgeMin, badge, badge, c.Success, Icons::Check, true);
			}
			else
			{
				char number[4];
				std::snprintf(number, sizeof(number), "%d", index + 1);
				TextBox(draw, Theme::Fonts().Small, badgeMin, badge, badge,
					selected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : c.TextSecondary, number, true);
			}

			const float textX = badgeMax.x + 12.0f * scale;
			Text(draw, Theme::Fonts().Body, ImVec2(textX, position.y + 9.0f * scale),
				selected ? c.TextPrimary : c.TextSecondary, name);
			Text(draw, Theme::Fonts().Small, ImVec2(textX, position.y + 30.0f * scale), c.TextMuted, hint);

			ImGui::SetCursorScreenPos(ImVec2(position.x, max.y + 6.0f * scale));
		}

		// bodyHeight 必须由调用方传入：GetContentRegionAvail().y 会一直算到窗口底部，
		// 而导航区应当止于底部操作栏之上，否则底部摘要块会压到操作栏上。
		void DrawNav(LauncherState& state, float scale, float bodyHeight)
		{
			const Theme::Palette& c = Theme::Colors();
			const float navWidth = Chrome::NavWidth * scale;
			const float height = bodyHeight;

			ImGui::PushStyleColor(ImGuiCol_ChildBg, c.NavBg);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * scale, 18.0f * scale));
			ImGui::BeginChild("##nav", ImVec2(navWidth, height),
				ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			const ImVec2 origin = ImGui::GetWindowPos();
			const float childWidth = ImGui::GetWindowSize().x;
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float contentWidth = ImGui::GetContentRegionAvail().x;
			ImDrawList* draw = ImGui::GetWindowDrawList();

			draw->AddLine(ImVec2(origin.x + childWidth, origin.y),
				ImVec2(origin.x + childWidth, origin.y + height), ImGui::GetColorU32(c.BorderSoft));

			// 引擎标识块
			{
				const float blockHeight = 58.0f * scale;
				draw->AddRectFilled(start, ImVec2(start.x + contentWidth, start.y + blockHeight),
					ImGui::GetColorU32(c.Surface), 10.0f * scale);

				const float boxSize = 38.0f * scale;
				const ImVec2 boxMin = ImVec2(start.x + 12.0f * scale, start.y + (blockHeight - boxSize) * 0.5f);
				draw->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize, boxMin.y + boxSize),
					ImGui::GetColorU32(c.AccentSoft), 9.0f * scale);
				if (!DrawLogoInBox(draw, boxMin, boxSize, 2.0f * scale))
					TextBox(draw, Theme::Fonts().Title, boxMin, boxSize, boxSize, c.Accent, Icons::Cubes, true);

				const float textX = boxMin.x + boxSize + 11.0f * scale;
				Text(draw, Theme::Fonts().Body, ImVec2(textX, start.y + 9.0f * scale), c.TextPrimary,
					Tr(S::EngineName, state.Lang));
				Text(draw, Theme::Fonts().Small, ImVec2(textX, start.y + 30.0f * scale), c.TextMuted,
					Tr(S::VersionLabel, state.Lang));

				ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + blockHeight + 20.0f * scale));
			}

			SectionLabel(Icons::ListUl, Tr(S::NavPipelineLabel, state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

			DrawNavItem(state, 0, Tr(S::NavProject, state.Lang), Tr(S::NavProjectHint, state.Lang), scale);
			DrawNavItem(state, 1, Tr(S::NavModules, state.Lang), Tr(S::NavModulesHint, state.Lang), scale);
			DrawNavItem(state, 2, Tr(S::NavSettings, state.Lang), Tr(S::NavSettingsHint, state.Lang), scale);
			DrawNavItem(state, 3, Tr(S::NavSummary, state.Lang), Tr(S::NavSummaryHint, state.Lang), scale);

			// 底部：当前装配摘要
			{
				const float blockHeight = 78.0f * scale;
				const ImVec2 position = ImVec2(start.x, origin.y + height - blockHeight - 18.0f * scale);

				draw->AddRectFilled(position, ImVec2(position.x + contentWidth, position.y + blockHeight),
					ImGui::GetColorU32(c.Surface), 10.0f * scale);

				const ImVec4 phaseColor = PhaseColor(state.Phase);
				draw->AddCircleFilled(ImVec2(position.x + 17.0f * scale, position.y + 21.0f * scale),
					4.0f * scale, ImGui::GetColorU32(phaseColor), 16);

				Text(draw, Theme::Fonts().Small, ImVec2(position.x + 29.0f * scale, position.y + 14.0f * scale),
					phaseColor, PhaseLabel(state.Phase, state.Lang));

				char summary[192];
				std::snprintf(summary, sizeof(summary), "%s · %d %s",
					BackendLabel(state.Backend),
					EnabledModuleCount(state),
					Tr(S::ModulesUnitLabel, state.Lang));
				Text(draw, Theme::Fonts().Small, ImVec2(position.x + 17.0f * scale, position.y + 40.0f * scale),
					c.TextSecondary, summary);

				const int blocking = BlockingModuleCount(state);
				if (blocking > 0)
				{
					char blocked[64];
					std::snprintf(blocked, sizeof(blocked), "%s %d",
						Tr(S::ModulesBlockingPrefix, state.Lang), blocking);
					Text(draw, Theme::Fonts().Small, ImVec2(position.x + 17.0f * scale, position.y + 57.0f * scale),
						c.Warning, blocked);
				}
			}

			ImGui::EndChild();
		}

		// ------------------------------------------------------------ 页面：项目

		void ProjectPage(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();

			PageHeader(Tr(S::ProjectTitle, state.Lang), Tr(S::ProjectHint, state.Lang), scale);

			const ImVec2 buttonSize = ImVec2(150.0f * scale, 38.0f * scale);
			if (GhostButton(Tr(S::ProjectNew, state.Lang), Icons::Plus, buttonSize, scale, true))
			{
				Actions::AppendLog(state, LogLevel::Info, "新建项目（界面演示）", "New project (UI demo)");
			}
			ImGui::SameLine();
			if (GhostButton(Tr(S::ProjectOpen, state.Lang), Icons::FolderOpen, buttonSize, scale, true))
			{
				Actions::AppendLog(state, LogLevel::Info, "打开项目对话框（界面演示）", "Open project dialog (UI demo)");
			}

			ImGui::Dummy(ImVec2(0.0f, 18.0f * scale));
			SectionLabel(Icons::Clock, Tr(S::ProjectRecent, state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));

			static char search[128] = "";
			ImGui::SetNextItemWidth(width - 2.0f * scale);
			ImGui::InputTextWithHint("##project-search", Tr(S::ProjectSearch, state.Lang), search, sizeof(search));

			ImGui::Dummy(ImVec2(0.0f, 14.0f * scale));

			const float rowHeight = 68.0f * scale;
			const std::string filter = search;

			for (std::size_t index = 0; index < state.Projects.size(); ++index)
			{
				ProjectEntry& project = state.Projects[index];
				if (!filter.empty() && project.Name.find(filter) == std::string::npos)
					continue;

				const ImVec2 position = ImGui::GetCursorScreenPos();
				const bool selected = (state.SelectedProject == static_cast<int>(index));

				ImGui::PushID(static_cast<int>(index));
				ImGui::InvisibleButton("##project", ImVec2(width, rowHeight));
				const bool hovered = ImGui::IsItemHovered();
				const bool clicked = ImGui::IsItemClicked();
				ImGui::PopID();

				ImDrawList* draw = ImGui::GetWindowDrawList();
				const ImVec2 max = ImVec2(position.x + width, position.y + rowHeight);
				const float rounding = 10.0f * scale;

				const ImVec4 background = selected ? c.AccentSoft : (hovered ? c.SurfaceHover : c.Surface);
				draw->AddRectFilled(position, max, ImGui::GetColorU32(background), rounding);
				draw->AddRect(position, max,
					ImGui::GetColorU32(selected ? WithAlpha(c.Accent, 0.5f) : c.Border), rounding, 0, 1.0f);

				if (selected)
					draw->AddRectFilled(position, ImVec2(position.x + 3.0f * scale, max.y),
						ImGui::GetColorU32(c.Accent), rounding);

				const float boxSize = 38.0f * scale;
				const ImVec2 boxMin = ImVec2(position.x + 16.0f * scale, position.y + (rowHeight - boxSize) * 0.5f);
				draw->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize, boxMin.y + boxSize),
					ImGui::GetColorU32(project.Valid ? c.SurfaceAlt : c.DangerSoft), 9.0f * scale);
				TextBox(draw, Theme::Fonts().Body, boxMin, boxSize, boxSize,
					project.Valid ? (selected ? c.Accent : c.TextSecondary) : c.Danger,
					Icons::FolderOpen, true);

				const float textX = boxMin.x + boxSize + 14.0f * scale;
				Text(draw, Theme::Fonts().Body, ImVec2(textX, position.y + 12.0f * scale),
					project.Valid ? c.TextPrimary : c.TextMuted, project.Name.c_str());

				const float nameWidth = TextSize(Theme::Fonts().Body, project.Name.c_str()).x;
				Text(draw, Theme::Fonts().Small,
					ImVec2(textX + nameWidth + 10.0f * scale, position.y + 16.0f * scale),
					c.TextMuted, project.EngineVersion.c_str());

				Text(draw, Theme::Fonts().Small, ImVec2(textX, position.y + 38.0f * scale),
					c.TextMuted, project.Path.c_str());

				const char* rightText = nullptr;
				std::string rightCombined;
				if (project.Valid)
				{
					rightCombined = project.LastOpened + "   " + project.Size;
					rightText = rightCombined.c_str();
				}
				else
				{
					rightText = Tr(S::ProjectInvalid, state.Lang);
				}

				const ImVec2 rightSize = TextSize(Theme::Fonts().Small, rightText);
				Text(draw, Theme::Fonts().Small,
					ImVec2(max.x - 18.0f * scale - rightSize.x, position.y + (rowHeight - rightSize.y) * 0.5f),
					project.Valid ? c.TextSecondary : c.Danger, rightText);

				if (clicked && project.Valid)
					state.SelectedProject = static_cast<int>(index);

				ImGui::SetCursorScreenPos(ImVec2(position.x, max.y + 10.0f * scale));
			}
		}

		// ------------------------------------------------------------ 页面：模块

		void BackendCards(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();

			struct Option
			{
				BackendType Type;
				S           Name;
				S           Desc;
				const char* Icon;
				ImVec4      Accent;
				ImVec4      Soft;
			};

			const Option options[] =
			{
				{ BackendType::OpenGL,    S::BackendOpenGLName, S::BackendOpenGLDesc, Icons::ThLarge, Hex(0x4ADE80), Hex(0x4ADE80, 0.14f) },
				{ BackendType::Vulkan,    S::BackendVulkanName, S::BackendVulkanDesc, Icons::Fire,    Hex(0xF97362), Hex(0xF97362, 0.14f) },
				{ BackendType::DirectX11, S::BackendDX11Name,   S::BackendDX11Desc,   Icons::Box,     Hex(0x58B8F5), Hex(0x58B8F5, 0.14f) },
			};

			const float spacing = 12.0f * scale;
			const int count = static_cast<int>(sizeof(options) / sizeof(options[0]));
			const float cardWidth = (width - spacing * (count - 1)) / count;
			const float cardHeight = 104.0f * scale;

			for (int index = 0; index < count; ++index)
			{
				const Option& option = options[index];
				const bool selected = (state.Backend == option.Type);

				ImGui::PushID(index);
				ImGui::SetCursorScreenPos(ImVec2(
					ImGui::GetCursorScreenPos().x + (index == 0 ? 0.0f : (index == count - 1 ? 0.0f : 0.0f)),
					ImGui::GetCursorScreenPos().y));
				const ImVec2 position = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##backend", ImVec2(cardWidth, cardHeight));
				const bool hovered = ImGui::IsItemHovered();
				const bool clicked = ImGui::IsItemClicked();
				ImGui::PopID();

				ImDrawList* draw = ImGui::GetWindowDrawList();
				const ImVec2 max = ImVec2(position.x + cardWidth, position.y + cardHeight);
				const float rounding = 11.0f * scale;

				const ImVec4 background = selected ? option.Soft : (hovered ? c.SurfaceHover : c.Surface);
				draw->AddRectFilled(position, max, ImGui::GetColorU32(background), rounding);
				draw->AddRect(position, max,
					ImGui::GetColorU32(selected ? WithAlpha(option.Accent, 0.6f) : c.Border),
					rounding, 0, selected ? 1.6f : 1.0f);

				const float boxSize = 34.0f * scale;
				const ImVec2 boxMin = ImVec2(position.x + 16.0f * scale, position.y + 16.0f * scale);
				draw->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize, boxMin.y + boxSize),
					ImGui::GetColorU32(option.Soft), 9.0f * scale);
				TextBox(draw, Theme::Fonts().Title, boxMin, boxSize, boxSize, option.Accent, option.Icon, true);

				Text(draw, Theme::Fonts().Body, ImVec2(boxMin.x + boxSize + 12.0f * scale, position.y + 22.0f * scale),
					selected ? c.TextPrimary : c.TextSecondary, Tr(option.Name, state.Lang));

				Text(draw, Theme::Fonts().Small, ImVec2(position.x + 16.0f * scale, position.y + 62.0f * scale),
					c.TextMuted, Tr(option.Desc, state.Lang));

				if (selected)
				{
					const ImVec2 checkPos = ImVec2(max.x - 26.0f * scale, position.y + 16.0f * scale);
					TextBox(draw, Theme::Fonts().Small, checkPos, 0.0f, 0.0f, option.Accent, Icons::CheckCircle);
				}

				if (clicked)
					state.Backend = option.Type;

				ImGui::SetCursorScreenPos(ImVec2(position.x + cardWidth + spacing, position.y));
			}

			ImGui::SetCursorScreenPos(ImVec2(
				ImGui::GetCursorScreenPos().x - (cardWidth + spacing) * count,
				ImGui::GetCursorScreenPos().y + cardHeight));
		}

		void WarningBanner(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();
			const float height = 48.0f * scale;
			const ImVec2 position = ImGui::GetCursorScreenPos();
			ImDrawList* draw = ImGui::GetWindowDrawList();

			draw->AddRectFilled(position, ImVec2(position.x + width, position.y + height),
				ImGui::GetColorU32(c.WarningSoft), 10.0f * scale);
			draw->AddRect(position, ImVec2(position.x + width, position.y + height),
				ImGui::GetColorU32(WithAlpha(c.Warning, 0.35f)), 10.0f * scale, 0, 1.0f);

			const float iconX = position.x + 18.0f * scale;
			TextBox(draw, Theme::Fonts().Body, ImVec2(iconX, position.y), 0.0f, height, c.Warning, Icons::Warning);

			const float textX = iconX + TextSize(Theme::Fonts().Body, Icons::Warning).x + 12.0f * scale;
			std::string message = std::string(Tr(S::SummaryBlockedNotice, state.Lang)) +
				"  (" + std::to_string(BlockingModuleCount(state)) + ")";
			TextBox(draw, Theme::Fonts().Body, ImVec2(textX, position.y), 0.0f, height, c.TextPrimary, message.c_str());

			ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + height));
		}

		void ModuleRow(LauncherState& state, ModuleEntry& module, int index, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();

			const float rowHeight = 88.0f * scale;
			const float toggleArea = 80.0f * scale;
			const ImVec2 position = ImGui::GetCursorScreenPos();
			const ImVec4 accent = ImVec4(module.Accent[0], module.Accent[1], module.Accent[2], 1.0f);
			const bool dimmed = !module.Enabled;

			ImGui::PushID(index);

			ImGui::InvisibleButton("##row", ImVec2(width - toggleArea, rowHeight));
			const bool hovered = ImGui::IsItemHovered();
			const bool clicked = ImGui::IsItemClicked();

			ImDrawList* draw = ImGui::GetWindowDrawList();
			const ImVec2 max = ImVec2(position.x + width, position.y + rowHeight);
			const float rounding = 10.0f * scale;

			const ImVec4 background = hovered ? c.SurfaceHover : c.Surface;
			draw->AddRectFilled(position, max,
				ImGui::GetColorU32(dimmed ? WithAlpha(background, 0.55f) : background), rounding);
			draw->AddRect(position, max, ImGui::GetColorU32(c.Border), rounding, 0, 1.0f);

			const float boxSize = 44.0f * scale;
			const ImVec2 boxMin = ImVec2(position.x + 16.0f * scale, position.y + (rowHeight - boxSize) * 0.5f);
			draw->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize, boxMin.y + boxSize),
				ImGui::GetColorU32(WithAlpha(accent, dimmed ? 0.08f : 0.16f)), 10.0f * scale);
			TextBox(draw, Theme::Fonts().Title, boxMin, boxSize, boxSize,
				dimmed ? WithAlpha(accent, 0.5f) : accent, module.Icon.c_str(), true);

			const float textX = boxMin.x + boxSize + 14.0f * scale;
			const float line1 = position.y + 14.0f * scale;
			const float line2 = position.y + 40.0f * scale;
			const float line3 = position.y + 61.0f * scale;

			const char* name = ModuleName(module, state.Lang);
			Text(draw, Theme::Fonts().Body, ImVec2(textX, line1), dimmed ? c.TextMuted : c.TextPrimary, name);
			float cursorX = textX + TextSize(Theme::Fonts().Body, name).x + 10.0f * scale;

			Text(draw, Theme::Fonts().Small, ImVec2(cursorX, line1 + 3.0f * scale), c.TextMuted,
				module.Version.c_str());
			cursorX += TextSize(Theme::Fonts().Small, module.Version.c_str()).x + 10.0f * scale;

			if (module.Required)
			{
				const char* required = Tr(S::ModulesRequired, state.Lang);
				const ImVec2 size = TextSize(Theme::Fonts().Small, required);
				const ImVec2 chipMin = ImVec2(cursorX, line1 + 1.0f * scale);
				const ImVec2 chipMax = ImVec2(chipMin.x + size.x + 14.0f * scale, chipMin.y + size.y + 6.0f * scale);
				draw->AddRectFilled(chipMin, chipMax, ImGui::GetColorU32(c.AccentSoft),
					(chipMax.y - chipMin.y) * 0.5f);
				TextBox(draw, Theme::Fonts().Small, ImVec2(chipMin.x + 7.0f * scale, chipMin.y),
					size.x, chipMax.y - chipMin.y, c.Accent, required);
			}

			Text(draw, Theme::Fonts().Small, ImVec2(textX, line2), c.TextSecondary,
				ModuleDescription(module, state.Lang));

			if (!module.NoteZh.empty())
			{
				Text(draw, Theme::Fonts().Small, ImVec2(textX, line3),
					dimmed ? WithAlpha(StatusColor(module.Status), 0.65f) : StatusColor(module.Status),
					ModuleNote(module, state.Lang));
			}
			else
			{
				const std::string dependencies = JoinDependencies(module);
				const std::string line = dependencies.empty()
					? std::string(Tr(S::ModulesNoDependency, state.Lang))
					: std::string(Tr(S::ModulesDependsOn, state.Lang)) + ": " + dependencies;
				Text(draw, Theme::Fonts().Small, ImVec2(textX, line3), c.TextMuted, line.c_str());
			}

			// 状态徽章：右对齐到开关区左侧
			{
				const char* label = Tr(StatusKey(module.Status), state.Lang);
				const float chipW = ChipWidth(label, scale);
				ImGui::SetCursorScreenPos(ImVec2(
					max.x - toggleArea - chipW - 8.0f * scale,
					position.y + 16.0f * scale));
				StatusBadge(module.Status, state.Lang, scale, dimmed);
			}

			// 开关（必需模块不可关闭）
			if (!module.Required)
			{
				ImGui::SetCursorScreenPos(ImVec2(
					max.x - toggleArea + 12.0f * scale,
					position.y + (rowHeight - 23.0f * scale) * 0.5f));
				if (ToggleSwitch("##toggle", module.Enabled, scale))
				{
					module.Enabled = !module.Enabled;
					Actions::AppendLog(state, LogLevel::Debug,
						std::string(module.Enabled ? "启用模块 " : "禁用模块 ") + module.Id,
						std::string(module.Enabled ? "Enabled module " : "Disabled module ") + module.Id);
				}
			}

			if (clicked)
				state.DetailModule = index;

			ImGui::PopID();
			ImGui::SetCursorScreenPos(ImVec2(position.x, max.y + 10.0f * scale));
		}

		void ModulesPage(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();

			PageHeader(Tr(S::ModulesTitle, state.Lang), Tr(S::ModulesHint, state.Lang), scale);

			SectionLabel(Icons::ThLarge, Tr(S::BackendTitle, state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));
			BackendCards(state, scale, width);
			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));

			if (ImFont* font = Theme::Fonts().Small)
				ImGui::PushFont(font);
			ImGui::TextColored(c.TextMuted, "%s", Tr(S::BackendHint, state.Lang));
			if (Theme::Fonts().Small)
				ImGui::PopFont();

			ImGui::Dummy(ImVec2(0.0f, 18.0f * scale));

			if (BlockingModuleCount(state) > 0)
			{
				WarningBanner(state, scale, width);
				ImGui::Dummy(ImVec2(0.0f, 14.0f * scale));
			}

			SectionLabel(Icons::PuzzlePiece, Tr(S::ModulesTitle, state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));

			for (std::size_t index = 0; index < state.Modules.size(); ++index)
				ModuleRow(state, state.Modules[index], static_cast<int>(index), scale, width);
		}

		// ------------------------------------------------------------ 页面：配置

		void SettingsPage(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();
			LaunchSettings& settings = state.Settings;

			PageHeader(Tr(S::SettingsTitle, state.Lang), Tr(S::SettingsHint, state.Lang), scale);

			// ---- 窗口 ----
			{
				// 高度交给内容决定：手写高度一旦估小就会把设置项裁掉。
				BeginPanel("##panel-window", ImVec2(width, 0.0f),
					Tr(S::SettingsWindowGroup, state.Lang), nullptr, scale, true);
				const float innerWidth = ImGui::GetContentRegionAvail().x;

				SectionLabel(Icons::ThLarge, Tr(S::SettingsResolution, state.Lang), scale);
				ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

				const ImVec2 chipOrigin = ImGui::GetCursorScreenPos();
				const char* presetNames[4] = { "1280 x 720", "1600 x 900", "1920 x 1080", "2560 x 1440" };
				const int presetValues[4][2] = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 } };
				for (int index = 0; index < 4; ++index)
				{
					const bool selected = (settings.Width == presetValues[index][0] &&
						settings.Height == presetValues[index][1]);
					char id[24];
					std::snprintf(id, sizeof(id), "##preset%d", index);
					if (SelectableChip(id, presetNames[index], selected, scale))
					{
						settings.Width = presetValues[index][0];
						settings.Height = presetValues[index][1];
					}
				}
				ImGui::SetCursorScreenPos(ImVec2(chipOrigin.x, chipOrigin.y + 38.0f * scale));

				ImGui::SetNextItemWidth(112.0f * scale);
				ImGui::InputInt("##width", &settings.Width, 0, 0);
				ImGui::SameLine();
				ImGui::TextColored(c.TextMuted, "×");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(112.0f * scale);
				ImGui::InputInt("##height", &settings.Height, 0, 0);

				ImGui::Dummy(ImVec2(0.0f, 2.0f * scale));

				if (ToggleRow("fullscreen", Tr(S::SettingsFullscreen, state.Lang),
					settings.Fullscreen, scale, innerWidth))
				{
					settings.Fullscreen = !settings.Fullscreen;
				}
				if (ToggleRow("vsync", Tr(S::SettingsVSync, state.Lang), settings.VSync, scale, innerWidth))
				{
					settings.VSync = !settings.VSync;
				}

				EndPanel(scale);
			}

			ImGui::Dummy(ImVec2(0.0f, 14.0f * scale));

			// ---- 日志 / 调试 并排 ----
			// 不再继续纵向堆叠：三块面板纵排会超出内容区高度，最后一块被视口底边切掉。
			{
				const ImVec2 origin = ImGui::GetCursorScreenPos();
				const float gap = 14.0f * scale;
				const float logWidth = (width - gap) * 0.58f;
				const float debugWidth = width - gap - logWidth;

				// ---- 日志 ----
				BeginPanel("##panel-log", ImVec2(logWidth, 0.0f),
					Tr(S::SettingsLogGroup, state.Lang), nullptr, scale, true);
				{
					SectionLabel(Icons::Terminal, Tr(S::SettingsLogLevel, state.Lang), scale);
					ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));

					const ImVec2 chipOrigin = ImGui::GetCursorScreenPos();
					const LogLevel levels[5] = { LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
						LogLevel::Warning, LogLevel::Error };
					for (int index = 0; index < 5; ++index)
					{
						char id[24];
						std::snprintf(id, sizeof(id), "##loglevel%d", index);
						if (SelectableChip(id, LogLevelLabel(levels[index], state.Lang),
							settings.Level == levels[index], scale))
						{
							settings.Level = levels[index];
						}
					}
					ImGui::SetCursorScreenPos(ImVec2(chipOrigin.x, chipOrigin.y + 38.0f * scale));
				}
				EndPanel(scale);

				// ---- 调试 ----
				ImGui::SetCursorScreenPos(ImVec2(origin.x + logWidth + gap, origin.y));
				BeginPanel("##panel-debug", ImVec2(debugWidth, 0.0f),
					Tr(S::SettingsDebugGroup, state.Lang), nullptr, scale, true);
				{
					const float innerWidth = ImGui::GetContentRegionAvail().x;

					if (ToggleRow("profiler", Tr(S::SettingsProfiler, state.Lang),
						settings.Profiler, scale, innerWidth))
					{
						settings.Profiler = !settings.Profiler;
					}

					ImGui::Dummy(ImVec2(0.0f, 2.0f * scale));
					ImGui::SetNextItemWidth(140.0f * scale);
					ImGui::InputInt("##port", &settings.DebugPort, 0, 0);
					ImGui::SameLine();
					ImGui::TextColored(c.TextMuted, "%s", Tr(S::SettingsDebugPort, state.Lang));
				}
				EndPanel(scale);
			}
		}

		// ------------------------------------------------------------ 页面：汇总

		void SummaryModuleList(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();
			const float rowHeight = 40.0f * scale;

			for (const ModuleEntry& module : state.Modules)
			{
				const ImVec2 position = ImGui::GetCursorScreenPos();
				ImDrawList* draw = ImGui::GetWindowDrawList();
				const bool dimmed = !module.Enabled;
				const ImVec4 accent = ImVec4(module.Accent[0], module.Accent[1], module.Accent[2], 1.0f);

				const float boxSize = 26.0f * scale;
				const ImVec2 boxMin = ImVec2(position.x, position.y + (rowHeight - boxSize) * 0.5f);
				draw->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize, boxMin.y + boxSize),
					ImGui::GetColorU32(WithAlpha(accent, dimmed ? 0.08f : 0.16f)), 7.0f * scale);
				TextBox(draw, Theme::Fonts().Small, boxMin, boxSize, boxSize,
					dimmed ? WithAlpha(accent, 0.5f) : accent, module.Icon.c_str(), true);

				const float textX = boxMin.x + boxSize + 12.0f * scale;
				const ImVec2 nameSize = TextSize(Theme::Fonts().Small, ModuleName(module, state.Lang));
				Text(draw, Theme::Fonts().Small, ImVec2(textX, position.y + (rowHeight - nameSize.y) * 0.5f),
					dimmed ? c.TextMuted : c.TextPrimary, ModuleName(module, state.Lang));

				if (dimmed)
				{
					Text(draw, Theme::Fonts().Small,
						ImVec2(position.x + width - TextSize(Theme::Fonts().Small,
							Tr(S::StatusDisabled, state.Lang)).x, position.y + (rowHeight - 15.0f * scale) * 0.5f),
						c.TextMuted, Tr(S::StatusDisabled, state.Lang));
					ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + rowHeight));
				}
				else
				{
					const char* label = Tr(StatusKey(module.Status), state.Lang);
					ImGui::SetCursorScreenPos(ImVec2(position.x + width - ChipWidth(label, scale),
						position.y + (rowHeight - 24.0f * scale) * 0.5f));
					StatusBadge(module.Status, state.Lang, scale, false);
					ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + rowHeight));
				}
			}
		}

		void LaunchStatusPanel(LauncherState& state, float scale, float width, float height)
		{
			const Theme::Palette& c = Theme::Colors();

			BeginPanel("##panel-status", ImVec2(width, height), nullptr, nullptr, scale);
			const float innerWidth = ImGui::GetContentRegionAvail().x;

			const bool active = (state.Phase != LaunchPhase::Idle);
			const bool failed = (state.Phase == LaunchPhase::Failed);

			const ImVec4 phaseColor = PhaseColor(state.Phase);

			// 阶段文字 + 百分比
			if (ImFont* font = Theme::Fonts().Title)
				ImGui::PushFont(font);
			const char* stage = active
				? Pick(state.Lang, state.StageZh, state.StageEn)
				: Tr(S::SummaryReadyToLaunch, state.Lang);
			ImGui::TextColored(failed ? c.Danger : c.TextPrimary, "%s", stage);
			if (Theme::Fonts().Title)
				ImGui::PopFont();

			char percent[16];
			std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(state.Progress * 100.0f));
			const ImVec2 percentSize = TextSize(Theme::Fonts().Body, percent);
			Text(ImGui::GetWindowDrawList(),
				Theme::Fonts().Body,
				ImVec2(ImGui::GetCursorScreenPos().x + innerWidth - percentSize.x,
					ImGui::GetCursorScreenPos().y - TextSize(Theme::Fonts().Title, stage).y - 2.0f * scale),
				phaseColor, percent);

			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
			ProgressStrip(state.Progress, phaseColor, ImVec2(innerWidth, 8.0f * scale), scale);
			ImGui::Dummy(ImVec2(0.0f, 10.0f * scale));

			// 运行中：时长与帧率
			if (state.Phase == LaunchPhase::Running)
			{
				if (ImFont* font = Theme::Fonts().Hero)
					ImGui::PushFont(font);
				char fps[32];
				std::snprintf(fps, sizeof(fps), "%.0f", state.RunningFrameRate);
				ImGui::TextColored(c.Success, "%s", fps);
				if (Theme::Fonts().Hero)
					ImGui::PopFont();

				ImGui::SameLine();
				if (ImFont* font = Theme::Fonts().Small)
					ImGui::PushFont(font);
				std::string meta = std::string("FPS   ·   ") +
					Tr(S::UptimeLabel, state.Lang) + " " + FormatClock(state.RunningTime);
				ImGui::TextColored(c.TextMuted, "%s", meta.c_str());
				if (Theme::Fonts().Small)
					ImGui::PopFont();
			}

			// 失败：错误详情
			if (failed)
			{
				const char* error = Pick(state.Lang, state.ErrorZh, state.ErrorEn);
				Text(ImGui::GetWindowDrawList(), Theme::Fonts().Small,
					ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y),
					c.Danger, Icons::TimesCircle);
				ImGui::Dummy(ImVec2(0.0f, 2.0f * scale));
				if (ImFont* font = Theme::Fonts().Small)
					ImGui::PushFont(font);
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + innerWidth);
				ImGui::TextColored(c.Danger, "%s", error);
				ImGui::PopTextWrapPos();
				if (Theme::Fonts().Small)
					ImGui::PopFont();
			}

			// 日志开关
			ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));
			{
				const ImVec2 rowPos = ImGui::GetCursorScreenPos();
				const float rowHeight = 30.0f * scale;
				ImGui::SetCursorScreenPos(ImVec2(rowPos.x + innerWidth - 42.0f * scale,
					rowPos.y + (rowHeight - 23.0f * scale) * 0.5f));
				if (ToggleSwitch("##showlog", state.ShowLogPanel, scale))
				{
					state.ShowLogPanel = !state.ShowLogPanel;
				}
				Text(ImGui::GetWindowDrawList(), Theme::Fonts().Small,
					ImVec2(rowPos.x, rowPos.y + (rowHeight - 15.0f * scale) * 0.5f),
					c.TextSecondary, Tr(S::LogPanelTitle, state.Lang));
				ImGui::SetCursorScreenPos(ImVec2(rowPos.x, rowPos.y + rowHeight));
			}

			// 日志列表
			if (state.ShowLogPanel)
			{
				const float listHeight = std::max(40.0f * scale, ImGui::GetContentRegionAvail().y - 4.0f * scale);
				ImGui::PushStyleColor(ImGuiCol_ChildBg, c.WindowBg);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * scale, 8.0f * scale));
				ImGui::BeginChild("##log", ImVec2(innerWidth, listHeight),
					ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();

				if (state.Log.empty())
				{
					if (ImFont* font = Theme::Fonts().Small)
						ImGui::PushFont(font);
					ImGui::TextColored(c.TextMuted, "%s", Tr(S::LogPanelEmpty, state.Lang));
					if (Theme::Fonts().Small)
						ImGui::PopFont();
				}
				else
				{
					if (ImFont* font = Theme::Fonts().Small)
						ImGui::PushFont(font);
					for (const LogLine& line : state.Log)
					{
						ImGui::TextColored(LogLevelColor(line.Level), "%6.2fs  %-7s  %s",
							line.Time, LogLevelLabel(line.Level, state.Lang),
							Pick(state.Lang, line.TextZh, line.TextEn));
					}
					if (state.Log.size() > 3)
						ImGui::SetScrollHereY(1.0f);
					if (Theme::Fonts().Small)
						ImGui::PopFont();
				}

				ImGui::EndChild();
			}

			EndPanel(scale);
		}

		void SummaryPage(LauncherState& state, float scale, float width)
		{
			const Theme::Palette& c = Theme::Colors();

			PageHeader(Tr(S::SummaryTitle, state.Lang), Tr(S::SummaryHint, state.Lang), scale);

			const float available = ImGui::GetContentRegionAvail().y;
			const bool hasStatus = (state.Phase != LaunchPhase::Idle);
			// 上限要足以容纳六个模块行（左）与标题、五行配置加校验徽章（右），
			// 否则内容会被面板底边裁掉。放不下时由内容区滚动，而不是裁切。
			const float panelHeight = std::min(360.0f * scale, available * (hasStatus ? 0.62f : 0.95f));
			const float gap = 14.0f * scale;

			// 左：模块清单；右：装配摘要
			const float rightWidth = 330.0f * scale;
			const float leftWidth = width - rightWidth - gap;

			{
				const ImVec2 origin = ImGui::GetCursorScreenPos();

				ImGui::SetCursorScreenPos(origin);
				BeginPanel("##panel-modules", ImVec2(leftWidth, panelHeight),
					Tr(S::SummaryModules, state.Lang), nullptr, scale);
				SummaryModuleList(state, scale, ImGui::GetContentRegionAvail().x);
				EndPanel(scale);

				ImGui::SetCursorScreenPos(ImVec2(origin.x + leftWidth + gap, origin.y));
				BeginPanel("##panel-config", ImVec2(rightWidth, panelHeight),
					Tr(S::SummaryConfig, state.Lang), nullptr, scale);

				KeyValueRow(Tr(S::SummaryBackend, state.Lang), BackendLabel(state.Backend), scale);
				ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

				const char* projectName = (state.SelectedProject >= 0 &&
					state.SelectedProject < static_cast<int>(state.Projects.size()))
					? state.Projects[static_cast<std::size_t>(state.SelectedProject)].Name.c_str()
					: Tr(S::SummaryNoProject, state.Lang);
				KeyValueRow(Tr(S::SummaryProject, state.Lang), projectName, scale);
				ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

				char resolution[48];
				std::snprintf(resolution, sizeof(resolution), "%d x %d %s",
					state.Settings.Width, state.Settings.Height,
					Tr(state.Settings.Fullscreen ? S::SummaryFullscreenOn : S::SummaryWindowedOn, state.Lang));
				KeyValueRow(Tr(S::SummaryWindow, state.Lang), resolution, scale);
				ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

				KeyValueRow(Tr(S::SummaryLogLevel, state.Lang),
					LogLevelLabel(state.Settings.Level, state.Lang), scale);
				ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));

				KeyValueRow(Tr(S::SummaryEstimate, state.Lang), "412 MB", scale);

				ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));
				Divider(scale);
				ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));

				const int blocking = BlockingModuleCount(state);
				const bool ready = (blocking == 0) && Actions::Validate(state);

				const float badgeHeight = 34.0f * scale;
				const ImVec2 badgeMin = ImGui::GetCursorScreenPos();
				const ImVec4 badgeColor = ready ? c.Success : c.Warning;
				ImDrawList* draw = ImGui::GetWindowDrawList();
				draw->AddRectFilled(badgeMin, ImVec2(badgeMin.x + ImGui::GetContentRegionAvail().x,
					badgeMin.y + badgeHeight), ImGui::GetColorU32(WithAlpha(badgeColor, 0.14f)), 9.0f * scale);

				const char* badgeText = ready
					? Tr(S::SummaryReadyToLaunch, state.Lang)
					: Tr(S::SummaryBlockedNotice, state.Lang);
				TextBox(draw, Theme::Fonts().Small,
					ImVec2(badgeMin.x + 12.0f * scale, badgeMin.y),
					0.0f, badgeHeight, badgeColor, ready ? Icons::CheckCircle : Icons::Warning);
				const float iconWidth = TextSize(Theme::Fonts().Small, Icons::Warning).x;
				TextBox(draw, Theme::Fonts().Small,
					ImVec2(badgeMin.x + 12.0f * scale + iconWidth + 10.0f * scale, badgeMin.y),
					0.0f, badgeHeight, c.TextPrimary, badgeText);

				ImGui::SetCursorScreenPos(ImVec2(badgeMin.x, badgeMin.y + badgeHeight));
				EndPanel(scale);

				ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + panelHeight + gap));
			}

			if (hasStatus)
			{
				const float statusHeight = std::max(280.0f * scale, available - panelHeight - gap);
				LaunchStatusPanel(state, scale, width, statusHeight);
			}
		}

		// ------------------------------------------------------------ 底部操作栏

		void DrawBottomBar(LauncherState& state, const HostCommands& host, float scale, ImVec2 display)
		{
			(void)host;
			const Theme::Palette& c = Theme::Colors();
			const float height = Chrome::BottomBarHeight * scale;

			ImGui::PushStyleColor(ImGuiCol_ChildBg, c.WindowBg);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Chrome::ContentPadding * scale, 0.0f));
			ImGui::BeginChild("##bottom", ImVec2(display.x, height),
				ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			const ImVec2 origin = ImGui::GetWindowPos();
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float contentWidth = ImGui::GetContentRegionAvail().x;
			ImDrawList* draw = ImGui::GetWindowDrawList();

			draw->AddLine(ImVec2(origin.x, origin.y), ImVec2(origin.x + display.x, origin.y),
				ImGui::GetColorU32(c.BorderSoft));

			// ---- 左侧状态 ----
			{
				const ImVec4 phaseColor = PhaseColor(state.Phase);
				const float centerY = start.y + (height - 6.0f * scale) * 0.5f;
				const char* icon = (state.Phase == LaunchPhase::Failed) ? Icons::TimesCircle
					: (state.Phase == LaunchPhase::Running) ? Icons::CheckCircle
					: (BlockingModuleCount(state) > 0) ? Icons::Warning
					: Icons::CheckCircle;

				const ImVec2 iconSize = TextSize(Theme::Fonts().Body, icon);
				Text(draw, Theme::Fonts().Body, ImVec2(start.x, centerY - iconSize.y * 0.5f), phaseColor, icon);

				float textX = start.x + iconSize.x + 10.0f * scale;
				const char* statusText = PhaseLabel(state.Phase, state.Lang);
				if (state.Phase == LaunchPhase::Idle)
				{
					statusText = (BlockingModuleCount(state) > 0)
						? Tr(S::ModulesBlockingPrefix, state.Lang)
						: Tr(S::SummaryReadyToLaunch, state.Lang);
				}

				const ImVec2 statusSize = TextSize(Theme::Fonts().Body, statusText);
				Text(draw, Theme::Fonts().Body, ImVec2(textX, centerY - statusSize.y * 0.5f),
					c.TextPrimary, statusText);
				textX += statusSize.x + 14.0f * scale;

				Text(draw, Theme::Fonts().Small, ImVec2(textX, centerY - 7.0f * scale), c.TextMuted, "·");
				textX += 12.0f * scale;

				std::string meta = std::string(BackendLabel(state.Backend)) + " · " +
					std::to_string(EnabledModuleCount(state)) + " " + Tr(S::ModulesUnitLabel, state.Lang);
				if (state.Phase == LaunchPhase::Running)
				{
					char fps[48];
					std::snprintf(fps, sizeof(fps), " · %.0f FPS · %s",
						state.RunningFrameRate, FormatClock(state.RunningTime).c_str());
					meta += fps;
				}
				Text(draw, Theme::Fonts().Small, ImVec2(textX, centerY - 7.0f * scale), c.TextSecondary, meta.c_str());
			}

			// ---- 右侧按钮 ----
			{
				const float buttonHeight = 38.0f * scale;
				const float buttonY = start.y + (height - buttonHeight) * 0.5f;

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f * scale, 0.0f));

				const bool onSummary = (state.Current == Step::Summary);
				const bool busy = (state.Phase == LaunchPhase::Validating ||
					state.Phase == LaunchPhase::Launching ||
					state.Phase == LaunchPhase::Running);

				const float wideWidth = 160.0f * scale;
				const float narrowWidth = 118.0f * scale;
				const float totalWidth = wideWidth + narrowWidth + 10.0f * scale;

				ImGui::SetCursorScreenPos(ImVec2(start.x + contentWidth - totalWidth, buttonY));

				const bool canGoBack = (state.Current != Step::Project) && !busy;
				if (GhostButton(Tr(S::ButtonBack, state.Lang), Icons::ArrowLeft,
					ImVec2(narrowWidth, buttonHeight), scale, canGoBack))
				{
					state.Current = static_cast<Step>(static_cast<int>(state.Current) - 1);
				}

				ImGui::SetCursorScreenPos(ImVec2(start.x + contentWidth - wideWidth, buttonY));

				if (busy)
				{
					if (GhostButton(Tr(S::ButtonStop, state.Lang), Icons::Stop,
						ImVec2(wideWidth, buttonHeight), scale, true))
					{
						Actions::Stop(state);
					}
				}
				else if (onSummary)
				{
					const bool launchable = Actions::Validate(state);
					const char* label = (state.Phase == LaunchPhase::Failed)
						? Tr(S::ButtonRetry, state.Lang)
						: Tr(S::ButtonLaunch, state.Lang);

					if (PrimaryButton(label, Icons::Rocket, ImVec2(wideWidth, buttonHeight), scale, launchable))
					{
						Actions::Launch(state);
					}
				}
				else
				{
					if (PrimaryButton(Tr(S::ButtonNext, state.Lang), Icons::ArrowRight,
						ImVec2(wideWidth, buttonHeight), scale, true))
					{
						state.Current = static_cast<Step>(static_cast<int>(state.Current) + 1);
					}
				}

				ImGui::PopStyleVar();
			}

			ImGui::EndChild();
		}

		// ------------------------------------------------------------ 模块详情

		void DrawModuleDetail(LauncherState& state, float scale)
		{
			if (state.DetailModule < 0 || state.DetailModule >= static_cast<int>(state.Modules.size()))
				return;

			const Theme::Palette& c = Theme::Colors();
			const ImGuiIO& io = ImGui::GetIO();
			const ModuleEntry& module = state.Modules[static_cast<std::size_t>(state.DetailModule)];
			const ImVec4 accent = ImVec4(module.Accent[0], module.Accent[1], module.Accent[2], 1.0f);

			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				state.DetailModule = -1;
				return;
			}

			// 遮罩层：必须晚于根窗口绘制，否则会被根窗口盖住。
			// 注意这里不能加 NoBringToFrontOnFocus —— 那会让遮罩永远留在根窗口之后，
			// 等于完全看不到变暗效果（根窗口自己带这个标志是对的，遮罩不行）。
			ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
			ImGui::SetNextWindowSize(io.DisplaySize);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Hex(0x05070A, 0.62f));
			ImGui::Begin("##dim", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNavFocus |
				ImGuiWindowFlags_NoSavedSettings);
			ImGui::End();
			ImGui::PopStyleColor();

			// 有状态说明的模块要多一行提示条，弹窗高度随之调整：
			// 固定高度要么给无说明的模块留出大片空白，要么把提示条裁掉。
			const float popupHeight = module.NoteZh.empty() ? 400.0f : 470.0f;
			const ImVec2 size = ImVec2(580.0f * scale, popupHeight * scale);
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
				ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::PushStyleColor(ImGuiCol_WindowBg, c.Surface);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(26.0f * scale, 24.0f * scale));
			ImGui::Begin("##module-detail", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			const float innerWidth = ImGui::GetContentRegionAvail().x;

			// 标题行
			{
				const ImVec2 position = ImGui::GetCursorScreenPos();
				ImDrawList* draw = ImGui::GetWindowDrawList();
				const float boxSize = 46.0f * scale;

				draw->AddRectFilled(position, ImVec2(position.x + boxSize, position.y + boxSize),
					ImGui::GetColorU32(WithAlpha(accent, 0.16f)), 11.0f * scale);
				TextBox(draw, Theme::Fonts().Hero, position, boxSize, boxSize, accent, module.Icon.c_str(), true);

				const float textX = position.x + boxSize + 16.0f * scale;
				Text(draw, Theme::Fonts().Title, ImVec2(textX, position.y + 2.0f * scale),
					c.TextPrimary, ModuleName(module, state.Lang));
				Text(draw, Theme::Fonts().Small, ImVec2(textX, position.y + 28.0f * scale),
					c.TextMuted, ModuleDescription(module, state.Lang));

				// 右上角关闭
				ImGui::SetCursorScreenPos(ImVec2(position.x + innerWidth - 30.0f * scale, position.y));
				if (IconButton("##close-detail", Icons::Times, ImVec2(30.0f * scale, 30.0f * scale),
					c.SurfaceHover, c.TextSecondary))
				{
					state.DetailModule = -1;
				}

				ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + boxSize + 18.0f * scale));
			}

			Divider(scale);
			ImGui::Dummy(ImVec2(0.0f, 10.0f * scale));

			KeyValueRow(Tr(S::ModulesFileName, state.Lang), module.FileName.c_str(), scale);
			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
			KeyValueRow(Tr(S::ModulesVersionLabel, state.Lang), module.Version.c_str(), scale);
			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
			KeyValueRow(Tr(S::ModulesStatusLabel, state.Lang), Tr(StatusKey(module.Status), state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
			KeyValueRow(Tr(S::ModulesProvides, state.Lang), ModuleProvides(module, state.Lang), scale);

			ImGui::Dummy(ImVec2(0.0f, 16.0f * scale));
			SectionLabel(Icons::Link, Tr(S::ModulesDependsOn, state.Lang), scale);
			ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));

			{
				const ImVec2 chipOrigin = ImGui::GetCursorScreenPos();
				if (module.DependsOn.empty())
				{
					if (ImFont* font = Theme::Fonts().Small)
						ImGui::PushFont(font);
					ImGui::TextColored(c.TextMuted, "%s", Tr(S::ModulesNoDependency, state.Lang));
					if (Theme::Fonts().Small)
						ImGui::PopFont();
				}
				else
				{
					for (const std::string& dependency : module.DependsOn)
						InfoChip(dependency.c_str(), scale);
				}
				ImGui::SetCursorScreenPos(ImVec2(chipOrigin.x, chipOrigin.y + 38.0f * scale));
			}

			if (!module.NoteZh.empty())
			{
				const ImVec4 noteColor = StatusColor(module.Status);
				const float noteHeight = 44.0f * scale;
				const ImVec2 position = ImGui::GetCursorScreenPos();
				ImDrawList* draw = ImGui::GetWindowDrawList();

				draw->AddRectFilled(position, ImVec2(position.x + innerWidth, position.y + noteHeight),
					ImGui::GetColorU32(WithAlpha(noteColor, 0.12f)), 9.0f * scale);

				TextBox(draw, Theme::Fonts().Small, ImVec2(position.x + 12.0f * scale, position.y),
					0.0f, noteHeight, noteColor, Icons::InfoCircle);
				const float iconWidth = TextSize(Theme::Fonts().Small, Icons::InfoCircle).x;
				TextBox(draw, Theme::Fonts().Small,
					ImVec2(position.x + 12.0f * scale + iconWidth + 10.0f * scale, position.y),
					0.0f, noteHeight, c.TextPrimary, ModuleNote(module, state.Lang));

				ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + noteHeight));
			}

			// 必须与上面的 Begin("##module-detail") 配对。
			// 漏掉这一句 ImGui 会在帧末检测到未配对的 Begin/End 并中止进程。
			ImGui::End();
		}
	}

	// ---------------------------------------------------------------- 入口

	void Draw(LauncherState& state, const HostCommands& host, float deltaTime)
	{
		(void)deltaTime;

		const float scale = Theme::Scale();
		const ImGuiIO& io = ImGui::GetIO();
		const ImVec2 display = io.DisplaySize;

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(display);
		ImGui::Begin("##root", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);

		const float titleHeight = Chrome::TitleBarHeight * scale;
		const float bottomHeight = Chrome::BottomBarHeight * scale;
		const float bodyHeight = std::max(0.0f, display.y - titleHeight - bottomHeight);

		DrawTitleBar(state, host, display, scale);

		ImGui::SetCursorPos(ImVec2(0.0f, titleHeight));
		DrawNav(state, scale, bodyHeight);

		ImGui::SetCursorPos(ImVec2(Chrome::NavWidth * scale, titleHeight));
		{
			const float contentWidth = display.x - Chrome::NavWidth * scale;
			const float padding = Chrome::ContentPadding * scale;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
			ImGui::BeginChild("##content", ImVec2(contentWidth, bodyHeight),
				ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
			ImGui::PopStyleVar();

			const float width = ImGui::GetContentRegionAvail().x;
			switch (state.Current)
			{
			case Step::Project:  ProjectPage(state, scale, width); break;
			case Step::Modules:  ModulesPage(state, scale, width); break;
			case Step::Settings: SettingsPage(state, scale, width); break;
			case Step::Summary:  SummaryPage(state, scale, width); break;
			default: break;
			}

			ImGui::EndChild();
		}

		ImGui::SetCursorPos(ImVec2(0.0f, display.y - bottomHeight));
		DrawBottomBar(state, host, scale, display);

		ImGui::End();

		DrawModuleDetail(state, scale);
	}
}
