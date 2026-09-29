#pragma once

#include <filesystem>

namespace Dark::Launcher::Paths
{
	// 可执行文件所在目录。
	const std::filesystem::path& ExecutableDirectory();

	// 依次在可执行文件目录、当前工作目录、仓库内常见位置查找资源。
	// 找不到时返回空路径。
	std::filesystem::path ResolveAsset(const std::filesystem::path& relative);

	// 界面字体（随程序分发的 Poppins），找不到时退回系统字体。
	std::filesystem::path ResolveUiFont();

	// 图标字体（Font Awesome 5 Free Solid）。找不到时返回空路径。
	std::filesystem::path ResolveIconFont();

	// 中文字体：优先随程序分发，否则使用系统微软雅黑 / 黑体。
	std::filesystem::path ResolveCjkFont();
}
