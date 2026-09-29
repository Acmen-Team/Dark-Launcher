#pragma once

namespace Dark::Launcher::Icons
{
	// Font Awesome 5 Free Solid 的码点。
	// 源码以 UTF-8 编译（见 Launcher.lua 的 /utf-8），因此 \uXXXX
	// 会直接写成 UTF-8 字节，ImGui 按 UTF-8 解析，无需手工转义。
	//
	// 图标字体以 MergeMode 合并进同一个字号的 ImFont，
	// 所以 "图标 + 文字" 可以在同一个字符串里混排。
	constexpr const char* Cube           = "\uf1b2";   // fa-cube
	constexpr const char* Cubes          = "\uf1b3";   // fa-cubes
	constexpr const char* LayerGroup     = "\uf5fd";   // fa-layer-group
	constexpr const char* Wrench         = "\uf0ad";   // fa-wrench
	constexpr const char* Rocket         = "\uf135";   // fa-rocket
	constexpr const char* Play           = "\uf04b";   // fa-play
	constexpr const char* Stop           = "\uf04d";   // fa-stop
	constexpr const char* Cog            = "\uf013";   // fa-cog
	constexpr const char* Cogs           = "\uf085";   // fa-cogs
	constexpr const char* Minus          = "\uf068";   // fa-minus
	constexpr const char* Expand         = "\uf065";   // fa-expand
	constexpr const char* Compress       = "\uf066";   // fa-compress
	constexpr const char* Times          = "\uf00d";   // fa-times
	constexpr const char* Check          = "\uf00c";   // fa-check
	constexpr const char* CheckCircle    = "\uf058";   // fa-check-circle
	constexpr const char* TimesCircle    = "\uf057";   // fa-times-circle
	constexpr const char* Warning        = "\uf071";   // fa-exclamation-triangle
	constexpr const char* InfoCircle     = "\uf05a";   // fa-info-circle
	constexpr const char* FolderOpen     = "\uf07c";   // fa-folder-open
	constexpr const char* Terminal       = "\uf120";   // fa-terminal
	constexpr const char* Sliders        = "\uf1de";   // fa-sliders-h
	constexpr const char* Microchip      = "\uf2db";   // fa-microchip
	constexpr const char* PaintBrush     = "\uf1fc";   // fa-paint-brush
	constexpr const char* Database       = "\uf1c0";   // fa-database
	constexpr const char* VolumeUp       = "\uf028";   // fa-volume-up
	constexpr const char* Scroll         = "\uf70e";   // fa-scroll
	constexpr const char* ProjectDiagram = "\uf542";   // fa-project-diagram
	constexpr const char* ArrowRight     = "\uf061";   // fa-arrow-right
	constexpr const char* ArrowLeft      = "\uf060";   // fa-arrow-left
	constexpr const char* AngleRight     = "\uf105";   // fa-angle-right
	constexpr const char* Plus           = "\uf067";   // fa-plus
	constexpr const char* Search         = "\uf002";   // fa-search
	constexpr const char* Copy           = "\uf0c5";   // fa-copy
	constexpr const char* Redo           = "\uf01e";   // fa-redo
	constexpr const char* Globe          = "\uf0ac";   // fa-globe
	constexpr const char* Bolt           = "\uf0e7";   // fa-bolt
	constexpr const char* Fire           = "\uf06d";   // fa-fire
	constexpr const char* Code           = "\uf121";   // fa-code
	constexpr const char* Box            = "\uf466";   // fa-box
	constexpr const char* PuzzlePiece    = "\uf12e";   // fa-puzzle-piece
	constexpr const char* Memory         = "\uf538";   // fa-memory
	constexpr const char* Hdd            = "\uf0a0";   // fa-hdd
	constexpr const char* Link           = "\uf0c1";   // fa-link
	constexpr const char* ListUl         = "\uf0ca";   // fa-list-ul
	constexpr const char* ThLarge        = "\uf009";   // fa-th-large
	constexpr const char* PowerOff       = "\uf011";   // fa-power-off
	constexpr const char* Eye            = "\uf06e";   // fa-eye
	constexpr const char* Star           = "\uf005";   // fa-star
	constexpr const char* ChartLine      = "\uf201";   // fa-chart-line
	constexpr const char* Clock          = "\uf017";   // fa-clock

	// 把所有用到的图标拼成一串，交给 ImFontGlyphRangesBuilder 精确构建字形范围，
	// 避免把整个私用区都塞进字体图集。
	inline const char* AllGlyphs()
	{
		return
			"\uf1b2\uf1b3\uf5fd\uf0ad\uf135\uf04b\uf04d\uf013\uf085\uf068"
			"\uf065\uf066\uf00d\uf00c\uf058\uf057\uf071\uf05a\uf07c\uf120"
			"\uf1de\uf2db\uf1fc\uf1c0\uf028\uf70e\uf542\uf061\uf060\uf105"
			"\uf067\uf002\uf0c5\uf01e\uf0ac\uf0e7\uf06d\uf121\uf466\uf12e"
			"\uf538\uf0a0\uf0c1\uf0ca\uf009\uf011\uf06e\uf005\uf201\uf017";
	}
}
