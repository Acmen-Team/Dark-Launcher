#pragma once

#include <string>
#include <vector>

namespace Dark::Launcher
{
	enum class Language
	{
		Chinese,
		English,
	};

	// 装配流程的四个步骤，顺序即左侧导航顺序。
	enum class Step
	{
		Project,
		Modules,
		Settings,
		Summary,
		Count,
	};

	enum class BackendType
	{
		OpenGL,
		Vulkan,
		DirectX11,
		Count,
	};

	enum class ModuleStatus
	{
		Ready,          // 可用
		Missing,        // 文件缺失
		Incompatible,   // 版本 / ABI 不匹配
		Disabled,       // 被用户关闭
	};

	// 启动流程状态机。界面只读取这些状态，具体推进由 Actions 负责。
	enum class LaunchPhase
	{
		Idle,
		Validating,
		Launching,
		Running,
		Failed,
	};

	enum class LogLevel
	{
		Trace,
		Debug,
		Info,
		Warning,
		Error,
	};

	struct ModuleEntry
	{
		std::string Id;              // 稳定标识，依赖引用用
		std::string Icon;            // Font Awesome 码点
		std::string NameZh;
		std::string NameEn;
		std::string DescZh;
		std::string DescEn;
		std::string NoteZh;          // 状态说明，例如版本不匹配的原因
		std::string NoteEn;
		std::string Version;
		std::string FileName;        // 对应 DLL 文件名
		std::string ProvidesZh;
		std::string ProvidesEn;

		ModuleStatus Status = ModuleStatus::Ready;
		bool Enabled = true;
		bool Required = false;       // 核心模块，不允许关闭
		std::vector<std::string> DependsOn;
		float Accent[4] = { 0.32f, 0.56f, 1.0f, 1.0f };
	};

	struct ProjectEntry
	{
		std::string Name;
		std::string Path;
		std::string LastOpened;
		std::string Scene;
		std::string EngineVersion;
		std::string Size;
		bool Valid = true;
	};

	struct LogLine
	{
		float Time = 0.0f;
		LogLevel Level = LogLevel::Info;
		std::string TextZh;
		std::string TextEn;
	};

	struct LaunchSettings
	{
		int  Width = 1600;
		int  Height = 900;
		bool Fullscreen = false;
		bool VSync = true;
		bool Profiler = false;
		LogLevel Level = LogLevel::Info;
		int  DebugPort = 27015;
	};

	struct LauncherState
	{
		// ---- 界面状态 ----
		Language Lang = Language::Chinese;
		Step     Current = Step::Project;

		// ---- 装配数据 ----
		std::vector<ModuleEntry>  Modules;
		std::vector<ProjectEntry> Projects;
		int         SelectedProject = 0;
		BackendType Backend = BackendType::OpenGL;
		LaunchSettings Settings;

		// ---- 启动状态 ----
		LaunchPhase Phase = LaunchPhase::Idle;
		float       Progress = 0.0f;
		float       PhaseTime = 0.0f;
		int         StageCursor = 0;   // 已进入的启动阶段序号
		std::string StageZh;
		std::string StageEn;
		std::string ErrorZh;
		std::string ErrorEn;
		std::vector<LogLine> Log;
		float RunningTime = 0.0f;
		float RunningFrameRate = 0.0f;

		// ---- 弹窗 ----
		int  DetailModule = -1;      // 打开的模块详情下标，-1 表示关闭
		bool ShowLogPanel = false;
	};

	// ---- 便捷查询（界面用，避免在绘制代码里写循环）----

	inline bool IsBlocking(const ModuleEntry& module)
	{
		return module.Enabled &&
			(module.Status == ModuleStatus::Missing || module.Status == ModuleStatus::Incompatible);
	}

	inline int EnabledModuleCount(const LauncherState& state)
	{
		int count = 0;
		for (const ModuleEntry& module : state.Modules)
		{
			if (module.Enabled)
				++count;
		}
		return count;
	}

	inline int BlockingModuleCount(const LauncherState& state)
	{
		int count = 0;
		for (const ModuleEntry& module : state.Modules)
		{
			if (IsBlocking(module))
				++count;
		}
		return count;
	}

	// ---- 按语言取内容字段 ----

	inline const char* Pick(Language language, const std::string& chinese, const std::string& english)
	{
		return language == Language::Chinese ? chinese.c_str() : english.c_str();
	}

	inline const char* ModuleName(const ModuleEntry& module, Language language)
	{
		return Pick(language, module.NameZh, module.NameEn);
	}

	inline const char* ModuleDescription(const ModuleEntry& module, Language language)
	{
		return Pick(language, module.DescZh, module.DescEn);
	}

	inline const char* ModuleNote(const ModuleEntry& module, Language language)
	{
		return Pick(language, module.NoteZh, module.NoteEn);
	}

	inline const char* ModuleProvides(const ModuleEntry& module, Language language)
	{
		return Pick(language, module.ProvidesZh, module.ProvidesEn);
	}
}
