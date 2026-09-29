#include "UI/MockData.h"

#include "UI/Icons.h"

namespace Dark::Launcher::MockData
{
	namespace
	{
		ModuleEntry MakeModule(
			const char* id,
			const char* icon,
			const char* nameZh, const char* nameEn,
			const char* descZh, const char* descEn,
			const char* version,
			const char* fileName,
			const char* providesZh, const char* providesEn,
			ModuleStatus status,
			bool enabled,
			bool required,
			std::vector<std::string> dependsOn,
			float r, float g, float b)
		{
			ModuleEntry module;
			module.Id         = id;
			module.Icon       = icon;
			module.NameZh     = nameZh;
			module.NameEn     = nameEn;
			module.DescZh     = descZh;
			module.DescEn     = descEn;
			module.Version    = version;
			module.FileName   = fileName;
			module.ProvidesZh = providesZh;
			module.ProvidesEn = providesEn;
			module.Status     = status;
			module.Enabled    = enabled;
			module.Required   = required;
			module.DependsOn  = std::move(dependsOn);
			module.Accent[0]  = r;
			module.Accent[1]  = g;
			module.Accent[2]  = b;
			module.Accent[3]  = 1.0f;
			return module;
		}
	}

	void PopulateModules(LauncherState& state)
	{
		state.Modules.clear();

		// 核心：必需，不可关闭。
		{
			ModuleEntry core = MakeModule(
				"core", Icons::Cube,
				"Dark 引擎核心", "Dark Engine Core",
				"应用生命周期、窗口、图层栈与事件系统",
				"Application lifecycle, window, layer stack and events",
				"0.1.0-alpha", "Dark.dll",
				"Application / Window / Layer", "Application / Window / Layer",
				ModuleStatus::Ready, true, true,
				{ "render", "resources", "tools" },
				0.30f, 0.55f, 1.00f);
			state.Modules.push_back(std::move(core));
		}

		// 渲染：依赖资源组件与工具库。
		{
			ModuleEntry render = MakeModule(
				"render", Icons::PaintBrush,
				"Dark-Render 渲染组件", "Dark-Render",
				"渲染后端抽象、渲染图与资源工厂",
				"Render backend abstraction, render graph and resource factories",
				"0.1.0-alpha", "Dark-Render.dll",
				"IRenderBackend / RenderGraph / TextureFactory",
				"IRenderBackend / RenderGraph / TextureFactory",
				ModuleStatus::Ready, true, false,
				{ "resources", "tools" },
				0.65f, 0.55f, 0.98f);
			state.Modules.push_back(std::move(render));
		}

		// 资源：依赖工具库。
		{
			ModuleEntry resources = MakeModule(
				"resources", Icons::Database,
				"Dark-Resources 资源组件", "Dark-Resources",
				"网格、材质、着色器与贴图的加载与缓存",
				"Loading and caching of meshes, materials, shaders and textures",
				"0.1.0-alpha", "Dark-Resources.dll",
				"IMesh / IMaterial / IShader / ITexture",
				"IMesh / IMaterial / IShader / ITexture",
				ModuleStatus::Ready, true, false,
				{ "tools" },
				0.18f, 0.83f, 0.75f);
			state.Modules.push_back(std::move(resources));
		}

		// 工具库：故意做成版本不匹配，用来演示 Incompatible 状态。
		{
			ModuleEntry tools = MakeModule(
				"tools", Icons::Wrench,
				"Dark-Tools 工具库", "Dark-Tools",
				"日志、数学与序列化基础库",
				"Logging, math and serialization foundation",
				"0.0.9", "Dark-Tools.dll",
				"Logger / Math / Serialize", "Logger / Math / Serialize",
				ModuleStatus::Incompatible, false, false,
				{},
				0.96f, 0.65f, 0.14f);
			tools.NoteZh = "需要 0.1.0，当前为 0.0.9（ABI 版本 2，期望 3）";
			tools.NoteEn = "Requires 0.1.0, found 0.0.9 (ABI version 2, expected 3)";
			state.Modules.push_back(std::move(tools));
		}

		// 脚本系统：独立仓库，无外部依赖，默认启用。
		{
			ModuleEntry script = MakeModule(
				"script", Icons::Scroll,
				"Dark-Script 脚本系统", "Dark-Script",
				"汇编器与虚拟机，提供过程式脚本能力",
				"Assembler and virtual machine for procedural scripting",
				"0.2.1", "Dark-Script.dll",
				"Assembler / VirtualMachine", "Assembler / VirtualMachine",
				ModuleStatus::Ready, true, false,
				{},
				0.96f, 0.45f, 0.72f);
			state.Modules.push_back(std::move(script));
		}

		// 未安装的模块：演示 Missing 状态。
		{
			ModuleEntry physics = MakeModule(
				"physics", Icons::Bolt,
				"物理组件", "Physics",
				"刚体、碰撞体与约束求解（尚未安装）",
				"Rigid bodies, colliders and constraint solver (not installed)",
				"—", "Dark-Physics.dll",
				"Rigidbody / Collider", "Rigidbody / Collider",
				ModuleStatus::Missing, false, false,
				{ "render" },
				0.98f, 0.57f, 0.24f);
			physics.NoteZh = "在 bin/ 中找不到 Dark-Physics.dll";
			physics.NoteEn = "Dark-Physics.dll was not found in bin/";
			state.Modules.push_back(std::move(physics));
		}

		state.DetailModule = -1;
	}

	void PopulateProjects(LauncherState& state)
	{
		state.Projects.clear();
		state.Projects.push_back({
			"Sandbox",
			"D:/Projects/Dark-Sandbox",
			"2026-09-24 18:20",
			"Scenes/Default.dscene",
			"0.1.0-alpha",
			"128 MB",
			true });

		state.Projects.push_back({
			"RendererLab",
			"D:/Projects/RendererLab",
			"2026-09-19 11:05",
			"Scenes/PBRShowcase.dscene",
			"0.1.0-alpha",
			"96 MB",
			true });

		state.Projects.push_back({
			"LegacyDemo",
			"E:/Archive/LegacyDemo",
			"2026-05-02 09:41",
			"Scenes/Main.dscene",
			"0.0.7",
			"42 MB",
			false });

		state.SelectedProject = 0;
	}
}
