#include "UI/TextCoverage.h"

#include "UI/Icons.h"
#include "UI/Strings.h"

namespace Dark::Launcher
{
	void CollectUiText(const LauncherState& state, std::vector<std::string>& out)
	{
		// 1) 字符串表里的全部界面文案（中英双语都要，英文用于拉丁字形兜底）。
		for (const Detail::StringEntry& entry : Detail::kStringTable)
		{
			out.emplace_back(entry.Chinese);
			out.emplace_back(entry.English);
		}

		// 2) 图标字体的码点。Font Awesome 把图标放在私用区，
		//    这些码点不属于任何常规字形范围，必须显式加入。
		out.emplace_back(Icons::AllGlyphs());

		// 3) 当前状态里的内容文案：模块、项目与日志。
		//    这部分将来会换成真实数据，机制不变。
		for (const ModuleEntry& module : state.Modules)
		{
			out.push_back(module.NameZh);
			out.push_back(module.NameEn);
			out.push_back(module.DescZh);
			out.push_back(module.DescEn);
			out.push_back(module.NoteZh);
			out.push_back(module.NoteEn);
			out.push_back(module.ProvidesZh);
			out.push_back(module.ProvidesEn);
			out.push_back(module.Version);
			out.push_back(module.FileName);
			for (const std::string& dependency : module.DependsOn)
				out.push_back(dependency);
		}

		for (const ProjectEntry& project : state.Projects)
		{
			out.push_back(project.Name);
			out.push_back(project.Path);
			out.push_back(project.Scene);
			out.push_back(project.EngineVersion);
			out.push_back(project.LastOpened);
			out.push_back(project.Size);
		}

		for (const LogLine& line : state.Log)
		{
			out.push_back(line.TextZh);
			out.push_back(line.TextEn);
		}

		// 4) 启动阶段文字。它们来自字符串表，已由第 1 步覆盖，
		//    但把它们拼接起来可以顺带覆盖数字与分隔符。
		out.emplace_back("%d%% 0123456789.:-x·()[]");
	}
}
