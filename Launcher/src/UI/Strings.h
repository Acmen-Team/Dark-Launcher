#pragma once

#include "UI/LauncherState.h"

#include <cstddef>

namespace Dark::Launcher
{
	// 界面文案表。凡是「界面外壳」上的文字都放这里，用一张 X-macro 同时生成
	// 枚举与查表数组，保证两侧永远一一对应。
	// 模块名、项目名这类「内容」放在 MockData 里，不进这张表。
#define DARK_LAUNCHER_STRINGS(X) \
	/* ---- 应用标题 ---- */ \
	X(AppTitle,              "Dark 启动器",                "Dark Launch") \
	X(AppTagline,            "模块化装配 · 按需启动",       "Assemble modules, launch on demand") \
	X(VersionLabel,          "0.1.0 alpha",                 "0.1.0 alpha") \
	X(EngineName,            "Dark Engine",                 "Dark Engine") \
	/* ---- 标题栏 ---- */ \
	X(LangChineseShort,      "中",                          "中") \
	X(LangEnglishShort,      "EN",                          "EN") \
	X(TitleMinimize,         "最小化",                       "Minimize") \
	X(TitleMaximize,         "最大化",                       "Maximize") \
	X(TitleRestore,          "向下还原",                     "Restore") \
	X(TitleClose,            "关闭",                         "Close") \
	/* ---- 左侧导航 ---- */ \
	X(NavProject,            "项目",                         "Project") \
	X(NavProjectHint,        "选择工程与场景",               "Pick a project") \
	X(NavModules,            "模块装配",                     "Modules") \
	X(NavModulesHint,        "挑选引擎组件",                 "Choose components") \
	X(NavSettings,           "启动配置",                     "Settings") \
	X(NavSettingsHint,       "窗口与调试",                   "Window & debug") \
	X(NavSummary,            "确认启动",                     "Launch") \
	X(NavSummaryHint,        "检查并运行",                   "Review & run") \
	X(NavPipelineLabel,      "装配流程",                     "Assembly pipeline") \
	/* ---- 渲染后端 ---- */ \
	X(BackendTitle,          "渲染后端",                     "Render backend") \
	X(BackendHint,           "选择 Dark-Render 使用的图形接口", "Graphics API used by Dark-Render") \
	X(BackendOpenGLName,     "OpenGL",                       "OpenGL") \
	X(BackendOpenGLDesc,     "兼容性最好，跨平台基线",        "Best compatibility, portable baseline") \
	X(BackendVulkanName,     "Vulkan",                       "Vulkan") \
	X(BackendVulkanDesc,     "显式同步，多线程友好",          "Explicit sync, multithread friendly") \
	X(BackendDX11Name,       "DirectX 11",                   "DirectX 11") \
	X(BackendDX11Desc,       "Windows 原生，调试工具完善",    "Native on Windows, mature tooling") \
	/* ---- 模块 ---- */ \
	X(ModulesTitle,          "引擎模块",                     "Engine modules") \
	X(ModulesHint,           "勾选需要加载的组件，依赖关系会自动校验", "Enable the components to load; dependencies are validated automatically") \
	X(ModulesRequired,       "必需",                         "Required") \
	X(ModulesOptional,       "可选",                         "Optional") \
	X(ModulesProvides,       "提供",                         "Provides") \
	X(ModulesDependsOn,      "依赖",                         "Depends on") \
	X(ModulesNoDependency,   "无外部依赖",                   "No external dependency") \
	X(ModulesDetail,         "模块详情",                     "Module detail") \
	X(ModulesClose,          "关闭",                         "Close") \
	X(ModulesFileName,       "文件",                         "File") \
	X(ModulesStatusLabel,    "状态",                         "Status") \
	X(ModulesVersionLabel,   "版本",                         "Version") \
	X(ModulesAllReady,       "所有已启用模块均可用",          "All enabled modules are available") \
	X(ModulesUnitLabel,      "个模块",                       "modules") \
	X(ModulesBlockingPrefix, "阻塞问题",                     "Blocking issues") \
	X(ModulesEnabledPrefix,  "已启用",                       "Enabled") \
	/* ---- 状态 ---- */ \
	X(StatusReady,           "就绪",                         "Ready") \
	X(StatusMissing,         "缺失",                         "Missing") \
	X(StatusIncompatible,    "版本不匹配",                   "Version mismatch") \
	X(StatusDisabled,        "已禁用",                       "Disabled") \
	/* ---- 项目页 ---- */ \
	X(ProjectTitle,          "选择项目",                     "Select a project") \
	X(ProjectHint,           "选择要打开的工程，或创建一个新工程", "Open an existing project or create a new one") \
	X(ProjectNew,            "新建项目",                     "New project") \
	X(ProjectOpen,           "打开项目…",                    "Open project…") \
	X(ProjectSearch,         "搜索项目…",                    "Search projects…") \
	X(ProjectRecent,         "最近项目",                     "Recent projects") \
	X(ProjectLastOpened,     "上次打开",                     "Last opened") \
	X(ProjectSizeLabel,      "体积",                         "Size") \
	X(ProjectInvalid,        "路径不可用",                   "Path unavailable") \
	X(ProjectSceneLabel,     "启动场景",                     "Startup scene") \
	X(ProjectEngineVersion,  "引擎版本",                     "Engine version") \
	/* ---- 配置页 ---- */ \
	X(SettingsTitle,         "启动配置",                     "Launch settings") \
	X(SettingsHint,          "这些参数会在启动时传给引擎",    "These values are handed to the engine at launch") \
	X(SettingsWindowGroup,   "窗口",                         "Window") \
	X(SettingsResolution,    "分辨率",                       "Resolution") \
	X(SettingsFullscreen,    "全屏启动",                     "Launch fullscreen") \
	X(SettingsVSync,         "垂直同步",                     "Vertical sync") \
	X(SettingsLogGroup,      "日志",                         "Logging") \
	X(SettingsLogLevel,      "日志级别",                     "Log level") \
	X(SettingsProfiler,      "启用性能分析",                 "Enable profiler") \
	X(SettingsDebugGroup,    "调试",                         "Debugging") \
	X(SettingsDebugPort,     "调试端口",                     "Debug port") \
	X(LogTrace,              "Trace",                        "Trace") \
	X(LogDebug,              "Debug",                        "Debug") \
	X(LogInfo,               "Info",                         "Info") \
	X(LogWarning,            "警告",                         "Warning") \
	X(LogError,              "错误",                         "Error") \
	/* ---- 汇总页 ---- */ \
	X(SummaryTitle,          "确认装配",                     "Review assembly") \
	X(SummaryHint,           "检查装配结果，然后启动引擎",    "Check the assembly, then launch the engine") \
	X(SummaryModules,        "模块清单",                     "Module list") \
	X(SummaryConfig,         "装配摘要",                     "Assembly summary") \
	X(SummaryBackend,        "渲染后端",                     "Render backend") \
	X(SummaryProject,        "目标项目",                     "Target project") \
	X(SummaryWindow,         "窗口尺寸",                     "Window size") \
	X(SummaryLogLevel,       "日志级别",                     "Log level") \
	X(SummaryReadyToLaunch,  "装配校验通过，可以启动",        "Assembly validated, ready to launch") \
	X(SummaryBlockedNotice,  "存在阻塞问题，解决后才能启动",  "Resolve blocking issues before launching") \
	X(SummaryNoProject,      "尚未选择项目",                 "No project selected") \
	X(SummaryEstimate,       "预计内存占用",                 "Estimated memory") \
	X(SummaryFullscreenOn,   "全屏",                         "Fullscreen") \
	X(SummaryWindowedOn,     "窗口",                         "Windowed") \
	/* ---- 启动状态 ---- */ \
	X(PhaseIdle,             "待启动",                       "Idle") \
	X(PhaseValidating,       "校验中",                       "Validating") \
	X(PhaseLaunching,        "启动中",                       "Launching") \
	X(PhaseRunning,          "运行中",                       "Running") \
	X(PhaseFailed,           "启动失败",                     "Failed") \
	X(StageValidate,         "正在校验模块依赖…",            "Validating module dependencies…") \
	X(StageLoadModules,      "正在加载引擎模块…",            "Loading engine modules…") \
	X(StageInitBackend,      "正在初始化渲染后端…",          "Initializing render backend…") \
	X(StageStartEditor,      "正在启动编辑器…",              "Starting the editor…") \
	X(StageDone,             "启动完成",                     "Launch complete") \
	X(RunningHeader,         "引擎已启动",                   "Engine is running") \
	X(UptimeLabel,           "运行时长",                     "Uptime") \
	X(FrameLabel,            "帧率",                         "Frame rate") \
	/* ---- 按钮与日志 ---- */ \
	X(ButtonNext,            "下一步",                       "Next") \
	X(ButtonBack,            "上一步",                       "Back") \
	X(ButtonLaunch,          "启动引擎",                     "Launch engine") \
	X(ButtonStop,            "停止引擎",                     "Stop engine") \
	X(ButtonRetry,           "重试",                         "Retry") \
	X(ButtonCopyLog,         "复制日志",                     "Copy log") \
	X(ButtonClearLog,        "清空",                         "Clear") \
	X(LogPanelTitle,         "启动日志",                     "Launch log") \
	X(LogPanelEmpty,         "暂无日志",                     "No log entries yet")

	enum class S
	{
#define DARK_LAUNCHER_STRING_ENUM(name, zh, en) name,
		DARK_LAUNCHER_STRINGS(DARK_LAUNCHER_STRING_ENUM)
#undef DARK_LAUNCHER_STRING_ENUM
		Count
	};

	namespace Detail
	{
		struct StringEntry
		{
			const char* Chinese;
			const char* English;
		};

		inline constexpr StringEntry kStringTable[] =
		{
#define DARK_LAUNCHER_STRING_ENTRY(name, zh, en) { zh, en },
			DARK_LAUNCHER_STRINGS(DARK_LAUNCHER_STRING_ENTRY)
#undef DARK_LAUNCHER_STRING_ENTRY
		};
	}

	// 取一条界面文案。
	inline const char* Tr(S key, Language language)
	{
		const std::size_t index = static_cast<std::size_t>(key);
		if (index >= static_cast<std::size_t>(S::Count))
			return "";
		const Detail::StringEntry& entry = Detail::kStringTable[index];
		return language == Language::Chinese ? entry.Chinese : entry.English;
	}
}
