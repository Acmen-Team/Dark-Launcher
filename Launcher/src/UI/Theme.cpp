#include "UI/Theme.h"

#include "Platform/Paths.h"
#include "UI/Icons.h"

#include <cmath>
#include <filesystem>

namespace Dark::Launcher::Theme
{
	namespace
	{
		ImVec4 Hex(unsigned int rgb, float alpha = 1.0f)
		{
			return ImVec4(
				static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f,
				static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
				static_cast<float>(rgb & 0xFFu) / 255.0f,
				alpha);
		}

		Palette MakePalette()
		{
			Palette palette = {};
			palette.WindowBg      = Hex(0x0D0F14);
			palette.NavBg         = Hex(0x11141A);
			palette.Surface       = Hex(0x181C24);
			palette.SurfaceAlt    = Hex(0x1E232C);
			palette.SurfaceHover  = Hex(0x252B36);
			palette.SurfaceActive = Hex(0x2C3341);
			palette.Border        = Hex(0x272D38);
			palette.BorderSoft    = Hex(0x1F242D);

			palette.TextPrimary   = Hex(0xE9ECF2);
			palette.TextSecondary = Hex(0x9BA4B4);
			palette.TextMuted     = Hex(0x626B7B);

			palette.Accent        = Hex(0x4C8DFF);
			palette.AccentHover   = Hex(0x6EA3FF);
			palette.AccentSoft    = Hex(0x4C8DFF, 0.16f);

			palette.Success       = Hex(0x3DD68C);
			palette.SuccessSoft   = Hex(0x3DD68C, 0.15f);
			palette.Warning       = Hex(0xF5A524);
			palette.WarningSoft   = Hex(0xF5A524, 0.15f);
			palette.Danger        = Hex(0xF2595A);
			palette.DangerSoft    = Hex(0xF2595A, 0.15f);
			palette.Info          = Hex(0x58B8F5);
			return palette;
		}

		Palette g_Palette = MakePalette();
		FontSet g_Fonts;
		LogoImage g_Logo;
		float   g_Scale = 1.0f;

		// 四个字号（100% DPI 下的逻辑像素）。
		constexpr float kFontSizes[4] = { 13.0f, 15.0f, 19.0f, 27.0f };
	}

	const Palette& Colors()
	{
		return g_Palette;
	}

	const FontSet& Fonts()
	{
		return g_Fonts;
	}

	const LogoImage& Logo()
	{
		return g_Logo;
	}

	void SetLogo(ImTextureID texture, float width, float height)
	{
		g_Logo.Texture = texture;
		g_Logo.Width = width;
		g_Logo.Height = height;
	}

	float Scale()
	{
		return g_Scale;
	}

	FontReport LoadFonts(float dpiScale, const std::vector<std::string>& uiText)
	{
		g_Scale = dpiScale;

		FontReport report;

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();

		const std::filesystem::path uiFontPath   = Paths::ResolveUiFont();
		const std::filesystem::path iconFontPath = Paths::ResolveIconFont();
		const std::filesystem::path cjkFontPath  = Paths::ResolveCjkFont();

		// 图标字形范围只收集界面上真正用到的码点，
		// 而不是把 Font Awesome 整个私用区塞进图集。
		static ImVector<ImWchar> iconRanges;
		{
			ImFontGlyphRangesBuilder builder;
			builder.AddText(Icons::AllGlyphs());
			builder.BuildRanges(&iconRanges);
		}

		// 中文字形范围：以「常用 2500 字」为基底，再把界面上真正出现的
		// 每个字符显式加进去。基底表只覆盖 97.97% 的常用字，
		// 「渲」「帧」「辨」这类字未必在内，漏掉就会渲染成方框。
		static ImVector<ImWchar> cjkRanges;
		{
			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
			for (const std::string& text : uiText)
				builder.AddText(text.c_str());
			builder.BuildRanges(&cjkRanges);
		}

		ImFont* loaded[4] = {};

		for (int index = 0; index < 4; ++index)
		{
			const float size = std::round(kFontSizes[index] * dpiScale);

			ImFont* base = nullptr;
			if (!uiFontPath.empty())
			{
				ImFontConfig config;
				config.OversampleH = 2;
				config.OversampleV = 1;
				config.PixelSnapH  = true;
				base = io.Fonts->AddFontFromFileTTF(
					uiFontPath.string().c_str(), size, &config, io.Fonts->GetGlyphRangesDefault());
			}

			if (!base)
				base = io.Fonts->AddFontDefault();

			// 合并图标字体。ImGui 会跳过已经存在的码点，
			// 所以和基础字体的 ASCII 重叠不会出问题。
			if (!iconFontPath.empty())
			{
				ImFontConfig merge;
				merge.MergeMode  = true;
				merge.PixelSnapH = true;
				merge.OversampleH = 1;
				io.Fonts->AddFontFromFileTTF(
					iconFontPath.string().c_str(), size, &merge, iconRanges.Data);
			}

			// 合并中文字形（范围里包含 ASCII，重复部分同样会被跳过）。
			if (!cjkFontPath.empty())
			{
				ImFontConfig merge;
				merge.MergeMode  = true;
				merge.PixelSnapH = true;
				merge.OversampleH = 1;
				io.Fonts->AddFontFromFileTTF(
					cjkFontPath.string().c_str(), size, &merge, cjkRanges.Data);
			}

			loaded[index] = base;
		}

		g_Fonts.Small = loaded[0];
		g_Fonts.Body  = loaded[1];
		g_Fonts.Title = loaded[2];
		g_Fonts.Hero  = loaded[3];

		// 字体图集要重建，让后端在下一帧重新上传纹理。
		io.Fonts->Build();

		report.UiFontLoaded   = !uiFontPath.empty();
		report.IconFontLoaded = !iconFontPath.empty();
		report.CjkFontLoaded  = !cjkFontPath.empty();

		// 抽查字形：合并后的 ImFont 应当同时含有图标码点与汉字码点，
		// 这是判断界面会不会出现方框的直接依据。
		if (ImFont* body = g_Fonts.Body)
		{
			report.IconGlyphAvailable = body->FindGlyphNoFallback(static_cast<ImWchar>(0xF1B2)) != nullptr;
			// 0x6E32 = 「渲」。这个词界面里一定会用到（Dark-Render 渲染组件），
			// 而它未必在「常用 2500 字」表内，正好用来验证补充范围是否生效。
			report.CjkGlyphAvailable = body->FindGlyphNoFallback(static_cast<ImWchar>(0x6E32)) != nullptr;
		}

		return report;
	}

	void ApplyStyle(float dpiScale)
	{
		g_Scale = dpiScale;

		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding    = 10.0f;
		style.ChildRounding     = 9.0f;
		style.FrameRounding     = 8.0f;
		style.PopupRounding     = 10.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding      = 8.0f;
		style.TabRounding       = 7.0f;

		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize  = 0.0f;
		style.FrameBorderSize  = 0.0f;
		style.PopupBorderSize  = 1.0f;

		// 根窗口自己控制内边距，这里给 0；各区块用 BeginChild 单独设置。
		style.WindowPadding    = ImVec2(0.0f, 0.0f);
		style.FramePadding     = ImVec2(12.0f, 7.0f);
		style.ItemSpacing      = ImVec2(10.0f, 10.0f);
		style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
		style.ScrollbarSize    = 10.0f;
		style.GrabMinSize      = 12.0f;
		style.DisabledAlpha    = 0.4f;

		style.ScaleAllSizes(dpiScale);

		const Palette& c = Colors();
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text]                  = c.TextPrimary;
		colors[ImGuiCol_TextDisabled]          = c.TextMuted;
		colors[ImGuiCol_WindowBg]              = c.WindowBg;
		colors[ImGuiCol_ChildBg]               = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_PopupBg]               = c.Surface;
		colors[ImGuiCol_Border]                = c.Border;
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_FrameBg]               = c.SurfaceAlt;
		colors[ImGuiCol_FrameBgHovered]        = c.SurfaceHover;
		colors[ImGuiCol_FrameBgActive]         = c.SurfaceActive;
		colors[ImGuiCol_TitleBg]               = c.NavBg;
		colors[ImGuiCol_TitleBgActive]         = c.NavBg;
		colors[ImGuiCol_TitleBgCollapsed]      = c.NavBg;
		colors[ImGuiCol_MenuBarBg]             = c.NavBg;
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_ScrollbarGrab]         = c.SurfaceHover;
		colors[ImGuiCol_ScrollbarGrabHovered]  = c.SurfaceActive;
		colors[ImGuiCol_ScrollbarGrabActive]   = c.Accent;
		colors[ImGuiCol_CheckMark]             = c.Accent;
		colors[ImGuiCol_SliderGrab]            = c.Accent;
		colors[ImGuiCol_SliderGrabActive]      = c.AccentHover;
		colors[ImGuiCol_Button]                = c.SurfaceAlt;
		colors[ImGuiCol_ButtonHovered]         = c.SurfaceHover;
		colors[ImGuiCol_ButtonActive]          = c.SurfaceActive;
		colors[ImGuiCol_Header]                = c.AccentSoft;
		colors[ImGuiCol_HeaderHovered]         = c.SurfaceHover;
		colors[ImGuiCol_HeaderActive]          = c.SurfaceActive;
		colors[ImGuiCol_Separator]             = c.BorderSoft;
		colors[ImGuiCol_SeparatorHovered]      = c.Border;
		colors[ImGuiCol_SeparatorActive]       = c.Accent;
		colors[ImGuiCol_ResizeGrip]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_Tab]                   = c.SurfaceAlt;
		colors[ImGuiCol_TabHovered]            = c.SurfaceHover;
		colors[ImGuiCol_TabSelected]           = c.AccentSoft;
		colors[ImGuiCol_TableHeaderBg]         = c.SurfaceAlt;
		colors[ImGuiCol_TableBorderStrong]     = c.Border;
		colors[ImGuiCol_TableBorderLight]      = c.BorderSoft;
		colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_TableRowBgAlt]         = Hex(0xFFFFFF, 0.015f);
		colors[ImGuiCol_TextSelectedBg]        = c.AccentSoft;
		colors[ImGuiCol_NavCursor]             = c.Accent;
		colors[ImGuiCol_ModalWindowDimBg]      = Hex(0x05070A, 0.62f);
	}
}
