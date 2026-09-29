#include "Actions.h"

#include "UI/MockData.h"
#include "UI/Strings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace Dark::Launcher::Actions
{
	namespace
	{
		std::ofstream g_LogFile;

		constexpr float kLaunchDuration = 5.0f;   // 假启动总时长（秒）
		constexpr std::size_t kMaxLogLines = 256;

		// 启动阶段：进度区间 + 对应的文案与日志。
		struct Stage
		{
			float Begin;
			float End;
			S     Title;
		};

		constexpr Stage kStages[] =
		{
			{ 0.00f, 0.15f, S::StageValidate },      // 校验依赖
			{ 0.15f, 0.60f, S::StageLoadModules },   // 加载模块
			{ 0.60f, 0.88f, S::StageInitBackend },   // 初始化后端
			{ 0.88f, 1.00f, S::StageStartEditor },   // 拉起编辑器
		};

		constexpr int kStageCount = static_cast<int>(sizeof(kStages) / sizeof(kStages[0]));

		const char* BackendName(BackendType backend)
		{
			switch (backend)
			{
			case BackendType::OpenGL:    return "OpenGL";
			case BackendType::Vulkan:    return "Vulkan";
			case BackendType::DirectX11: return "DirectX 11";
			default:                     return "Unknown";
			}
		}

		int CurrentStageIndex(float progress)
		{
			for (int index = 0; index < kStageCount; ++index)
			{
				if (progress < kStages[index].End)
					return index;
			}
			return kStageCount - 1;
		}

		void SetStageText(LauncherState& state, int stageIndex)
		{
			if (stageIndex < 0 || stageIndex >= kStageCount)
				return;
			state.StageZh = Tr(kStages[stageIndex].Title, Language::Chinese);
			state.StageEn = Tr(kStages[stageIndex].Title, Language::English);
		}

		// 进入某个阶段时补上对应的日志行。
		void EmitStageLog(LauncherState& state, int stageIndex)
		{
			switch (stageIndex)
			{
			case 0:
				AppendLog(state, LogLevel::Info,
					"校验 " + std::to_string(EnabledModuleCount(state)) + " 个模块的依赖与版本",
					"Validating dependencies and versions of " + std::to_string(EnabledModuleCount(state)) + " modules");
				break;

			case 1:
				for (const ModuleEntry& module : state.Modules)
				{
					if (!module.Enabled)
						continue;
					AppendLog(state, LogLevel::Debug,
						"加载 " + module.FileName + " (" + module.Version + ")",
						"Loaded " + module.FileName + " (" + module.Version + ")");
				}
				break;

			case 2:
				AppendLog(state, LogLevel::Info,
					std::string("初始化 ") + BackendName(state.Backend) + " 渲染后端",
					std::string("Initializing ") + BackendName(state.Backend) + " render backend");
				break;

			case 3:
				AppendLog(state, LogLevel::Info,
					"启动编辑器并移交窗口",
					"Starting the editor and handing over the window");
				break;

			default:
				break;
			}
		}
	}

	void AppendLog(LauncherState& state, LogLevel level,
		const std::string& chinese, const std::string& english)
	{
		LogLine line;
		line.Level  = level;
		line.TextZh = chinese;
		line.TextEn = english;
		line.Time   = state.PhaseTime;

		// 用已运行时间做时间戳，看起来更像真实日志。
		if (state.Phase == LaunchPhase::Running)
			line.Time = state.RunningTime;

		state.Log.push_back(std::move(line));
		if (state.Log.size() > kMaxLogLines)
			state.Log.erase(state.Log.begin(), state.Log.begin() + (state.Log.size() - kMaxLogLines));

		if (g_LogFile.is_open())
		{
			static const char* kLevelNames[] = { "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR" };
			const int levelIndex = static_cast<int>(level);
			const char* levelName = (levelIndex >= 0 && levelIndex < 5) ? kLevelNames[levelIndex] : "INFO ";

			char stamp[24];
			std::snprintf(stamp, sizeof(stamp), "%7.2fs", line.Time > 0.0f ? line.Time : state.PhaseTime);

			// 逐行落盘：进程可能被强杀，不能依赖退出时的 flush。
			g_LogFile << '[' << stamp << "] [" << levelName << "] " << english << '\n';
			g_LogFile.flush();
		}
	}

	void OpenLogFile(const std::string& path)
	{
		g_LogFile.open(path, std::ios::out | std::ios::trunc);
	}

	void CloseLogFile()
	{
		if (g_LogFile.is_open())
			g_LogFile.close();
	}

	void RefreshModules(LauncherState& state)
	{
		// 真实实现：扫描 bin/ 下的 DLL，读取版本与 ABI 号。
		MockData::PopulateModules(state);
	}

	void RefreshProjects(LauncherState& state)
	{
		// 真实实现：读取配置文件里的最近项目列表。
		MockData::PopulateProjects(state);
	}

	bool Validate(LauncherState& state)
	{
		// 真实实现：这里做版本号、ABI 版本与依赖闭包检查。
		const bool hasProject = state.SelectedProject >= 0 &&
			state.SelectedProject < static_cast<int>(state.Projects.size()) &&
			state.Projects[static_cast<std::size_t>(state.SelectedProject)].Valid;

		return hasProject && BlockingModuleCount(state) == 0;
	}

	void Launch(LauncherState& state)
	{
		if (!Validate(state))
			return;

		state.Phase       = LaunchPhase::Validating;
		state.Progress    = 0.0f;
		state.PhaseTime   = 0.0f;
		state.StageCursor = 0;
		state.RunningTime = 0.0f;
		state.RunningFrameRate = 0.0f;
		state.ErrorZh.clear();
		state.ErrorEn.clear();
		state.Log.clear();
		state.ShowLogPanel = true;

		SetStageText(state, 0);
		EmitStageLog(state, 0);
	}

	void Stop(LauncherState& state)
	{
		if (state.Phase == LaunchPhase::Idle)
			return;

		AppendLog(state, LogLevel::Info, "已停止引擎", "Engine stopped");

		state.Phase       = LaunchPhase::Idle;
		state.Progress    = 0.0f;
		state.PhaseTime   = 0.0f;
		state.StageCursor = 0;
		state.RunningTime = 0.0f;
		state.RunningFrameRate = 0.0f;
		state.StageZh.clear();
		state.StageEn.clear();
		state.ErrorZh.clear();
		state.ErrorEn.clear();
	}

	void Tick(LauncherState& state, float deltaTime)
	{
		if (state.Phase == LaunchPhase::Running)
		{
			state.RunningTime += deltaTime;
			// 假帧率，让“运行中”状态看起来是活的。
			state.RunningFrameRate = 143.0f + std::sin(state.RunningTime * 2.4f) * 5.0f;
			return;
		}

		if (state.Phase != LaunchPhase::Validating && state.Phase != LaunchPhase::Launching)
			return;

		state.PhaseTime += deltaTime;
		state.Progress = std::min(1.0f, state.Progress + deltaTime / kLaunchDuration);

		const int stageIndex = CurrentStageIndex(state.Progress);

		// 跨过阶段边界：更新阶段文字并补日志。
		if (stageIndex != state.StageCursor)
		{
			state.StageCursor = stageIndex;
			SetStageText(state, stageIndex);
			EmitStageLog(state, stageIndex);

			if (stageIndex >= 1)
				state.Phase = LaunchPhase::Launching;

			// 演示用的失败路径：DirectX 11 后端在真实的 RenderBackendType 里并不存在
			// （枚举目前只有 OpenGL 与 Vulkan），这里如实模拟成初始化失败，
			// 好让「失败态」界面也有真实数据可看。
			if (state.Backend == BackendType::DirectX11 && stageIndex >= 2)
			{
				state.Phase = LaunchPhase::Failed;
				state.ErrorZh = "DirectX 11 后端尚未实现：RenderBackendType 目前只有 OpenGL 与 Vulkan";
				state.ErrorEn = "DirectX 11 backend is not implemented: RenderBackendType only defines OpenGL and Vulkan";
				AppendLog(state, LogLevel::Error, state.ErrorZh, state.ErrorEn);
				return;
			}
		}

		if (state.Progress >= 1.0f)
		{
			state.Phase = LaunchPhase::Running;
			state.RunningTime = 0.0f;
			state.StageZh = Tr(S::StageDone, Language::Chinese);
			state.StageEn = Tr(S::StageDone, Language::English);
			AppendLog(state, LogLevel::Info,
				"引擎启动完成，窗口已移交给编辑器",
				"Engine started, window handed over to the editor");
		}
	}
}
