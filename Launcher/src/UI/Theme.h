#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace Dark::Launcher::Theme
{
	struct Palette
	{
		ImVec4 WindowBg;
		ImVec4 NavBg;
		ImVec4 Surface;
		ImVec4 SurfaceAlt;
		ImVec4 SurfaceHover;
		ImVec4 SurfaceActive;
		ImVec4 Border;
		ImVec4 BorderSoft;
		ImVec4 TextPrimary;
		ImVec4 TextSecondary;
		ImVec4 TextMuted;
		ImVec4 Accent;
		ImVec4 AccentHover;
		ImVec4 AccentSoft;
		ImVec4 Success;
		ImVec4 SuccessSoft;
		ImVec4 Warning;
		ImVec4 WarningSoft;
		ImVec4 Danger;
		ImVec4 DangerSoft;
		ImVec4 Info;
	};

	const Palette& Colors();

	// 按 DPI 缩放后加载的四个字号。
	// 图标与中文字形以 MergeMode 合并进每个字号，所以可以混排。
	struct FontSet
	{
		ImFont* Small = nullptr;   // 13px  标签、徽章
		ImFont* Body  = nullptr;   // 15px  正文
		ImFont* Title = nullptr;   // 19px  区块标题
		ImFont* Hero  = nullptr;   // 27px  大标题
	};

	const FontSet& Fonts();

	// 品牌图标（Dark logo）。
	// 图片缺失时不显示，界面回退到内置的立方体字形，所以这不是必需资源。
	struct LogoImage
	{
		ImTextureID Texture = 0;
		float Width = 0.0f;
		float Height = 0.0f;

		bool Valid() const { return Texture != 0 && Width > 0.0f && Height > 0.0f; }
	};

	const LogoImage& Logo();
	void SetLogo(ImTextureID texture, float width, float height);

	// 窗口 DPI 缩放系数。自绘控件里手工算的像素尺寸需要乘它。
	float Scale();

	// 字体加载结果。用于在启动时把「字体是否齐全」明确报出来，
	// 否则字形缺失只会表现为界面上的方框，很难定位。
	struct FontReport
	{
		bool UiFontLoaded = false;        // Poppins 或系统回退字体
		bool IconFontLoaded = false;      // Font Awesome
		bool CjkFontLoaded = false;       // 中文字体
		bool IconGlyphAvailable = false;  // 抽查一个图标码点是否真的有字形
		bool CjkGlyphAvailable = false;   // 抽查一个汉字是否真的有字形

		bool Complete() const
		{
			return IconFontLoaded && CjkFontLoaded && IconGlyphAvailable && CjkGlyphAvailable;
		}
	};

	// 加载字体，并返回覆盖情况。
	//
	// uiText 是界面上会用到的全部文字（见 CollectUiText）。中文字形范围
	// 以「常用 2500 字」为基底，再逐个加入 uiText 里出现的字符，
	// 这样界面上的每个字都一定有字形，不会退化成方框。
	FontReport LoadFonts(float dpiScale, const std::vector<std::string>& uiText);

	// 应用配色、圆角与间距。
	void ApplyStyle(float dpiScale);
}
