# Dark-Launcher

Dark 引擎的**启动器**（界面标题：Dark Launch / Dark 启动器）。
目标是让用户挑选需要的引擎模块与渲染后端，
再把引擎拉起来。

## 当前状态：界面阶段

界面部分已经完整实现，**功能尚未接线**。
所有「选择模块、校验依赖、启动引擎」的行为都是假数据驱动的演示流程，
不加载任何 DLL、不启动任何进程。

界面阶段刻意与功能解耦：UI 只依赖 `LauncherState` 与 `Actions` 两组接口，
后续接入真实功能时只需要重写 `Actions.cpp`，界面代码一行都不用改。

## 目录结构

```
Launcher/
  Content/Fonts/           界面字体（随程序分发）
  src/
    Main.cpp               窗口 / D3D11 / ImGui 生命周期与主循环
    Actions.h/.cpp         ★ 界面与真实功能之间唯一的接口（当前为假实现）
    Platform/
      Window.h/.cpp        无边框窗口（自绘标题栏、系统拖动与缩放）
      WindowChrome.h       窗口外壳尺寸常量（100% DPI 逻辑像素）
      D3D11Context.h/.cpp  仅用于呈现 ImGui 的 D3D11 上下文
      Paths.h/.cpp         资源与字体的查找
    UI/
      LauncherState.h      界面状态模型（模块表、项目、启动状态机）
      LauncherUI.h/.cpp    界面主体：标题栏 / 导航 / 四个页面 / 状态栏 / 弹窗
      Widgets.h/.cpp       自绘控件（徽章、开关、进度条、按钮）
      Theme.h/.cpp         配色、字体加载与 ImGui 样式
      Strings.h            中英双语文案表（X-macro，枚举与查表同源）
      Icons.h              Font Awesome 码点
      MockData.h/.cpp      演示用假数据
      TextCoverage.h/.cpp  收集界面实际用到的文字，用于构建字形范围
```

## 界面构成

单窗口三段式布局，四个步骤：

1. **项目** — 最近项目列表、搜索、新建/打开
2. **模块装配** — 渲染后端卡片 + 引擎模块列表（状态徽章、依赖、开关）
3. **启动配置** — 窗口 / 日志 / 调试三组参数
4. **确认启动** — 模块清单、装配摘要、启动状态与日志面板

启动状态机（`LaunchPhase`）：`Idle → Validating → Launching → Running / Failed`。
模块状态（`ModuleStatus`）：`Ready / Missing / Incompatible / Disabled`。
这些状态目前都可以用界面直接触发，便于确认交互与视觉。

演示用的失败路径：选择 **DirectX 11** 后端再启动，会走到 `Failed` 状态。
这不是随意的假失败——真实的 `DRender::RenderBackendType` 目前只有
`OpenGL` 与 `Vulkan`，界面把 DirectX 11 列出来本身就是待补的缺口。

## 界面资源

`Launcher/Content/` 下是随程序分发的资源，构建后会自动复制到输出目录：

| 文件 | 用途 |
|---|---|
| `Fonts/Poppins-Medium.ttf` | 界面拉丁文字 |
| `Fonts/fa-solid-900.ttf` | 图标（Font Awesome 5 Free Solid） |
| `Textures/DarkLogo.png` | 品牌图标，用于标题栏与导航头部 |

中文字形使用系统字体（微软雅黑 / 黑体），不随程序分发。
PNG 解码走 Windows 自带的 WIC，不引入第三方解码库。

**品牌图标必须用浅色版。** 界面底色是 `#11141A`（标题栏）与 `#181C24`（导航块）
这类深色，所以取自 `Dark-Editor/Content/Textures` 的 **`DarkWhiteIcon.png`**。
`DarkBlackIcon.png` 在深色底上几乎不可见——两者形状完全相同，只是纯黑与纯白。

图标属于可选资源：缺失时界面回退到内置的立方体字形，并写一条警告日志，不影响运行。
另外源图是 32×32，在 150% DPI 下会放大到约 54px；若有 64×64 或 128×128 的版本会更锐利。

## 构建

与仓库内其他组件一致，用 premake5 生成 VS2022 工程：

```
scripts\GenerateProjects-VS2022.bat
```

产物位于 `bin/<Config>-windows-x86_64/Launcher/`，
构建后会把 `Content` 一并复制过去，因此可执行文件可以独立运行。

界面源码是 UTF-8 中英双语，工程里显式加了 `/utf-8`，
否则 MSVC 会按系统代码页解释字符串，图标码点也会错乱。

## 命令行参数

| 参数 | 说明 |
|---|---|
| `--console` | 保留控制台窗口（默认隐藏，工程类型是 ConsoleApp 便于看输出） |
| `--log <path>` | 把启动日志同时写入文件 |
| `--backend <opengl\|vulkan\|dx11>` | 预设渲染后端，便于快捷方式或 CI 冒烟测试 |

启动时会向标准输出打印一行字体与字形自检结果，例如：

```
[Dark-Launcher] dpi=1.500 uiFont=1 iconFont=1 cjkFont=1 iconGlyph=1 cjkGlyph=1
```

字体缺字形时界面只会显示方框，很难反查，所以这里主动报出来。

## 实现要点

**无边框窗口。** 窗口保留 `WS_OVERLAPPEDWINDOW` 以获得系统阴影与边缘缩放，
但 `WM_NCCALCSIZE` 返回 0 去掉全部非客户区，标题栏与边框完全由 ImGui 绘制。
拖动、Aero Snap、双击最大化通过 `WM_NCHITTEST` 返回 `HTCAPTION` 交回系统处理，
不需要手写拖动逻辑。

注意：`CreateWindowExW` 期间系统只发一次 `wParam == FALSE` 的 `WM_NCCALCSIZE`，
那一次不负责重算非客户区，所以创建后必须用 `SWP_FRAMECHANGED` 强制补发一次
`wParam == TRUE` 的调用，否则窗口会一直带着原生边框。

**字体与字形范围。** 四种字号，每种字号把 Poppins（拉丁）、
Font Awesome（图标）与系统中文字体以 `MergeMode` 合并进同一个 `ImFont`，
因此「图标 + 中文 + 英文」可以在同一个字符串里混排。

中文字形范围以 ImGui 自带的「常用 2500 字」为基底，再逐个加入
`CollectUiText` 收集到的界面实际用字。基底表只覆盖 97.97% 的常用字，
「渲」「帧」「辨」这类字未必在内，漏掉就会渲染成方框。

**DPI。** 进程按 `PER_MONITOR_AWARE_V2` 感知 DPI，字号与样式按
`GetDpiForWindow` 缩放，因此在高分屏上是清晰的。

## 后续接线

`Actions.cpp` 里的五个函数就是全部接线点：

| 函数 | 真实实现要做的事 |
|---|---|
| `RefreshModules` | 扫描 `bin/` 下的 DLL，读取版本与 ABI 号 |
| `RefreshProjects` | 读取最近项目列表 |
| `Validate` | 版本 / 依赖闭包 / ABI 校验 |
| `Launch` | `LoadLibrary` 各模块，或 `CreateProcess` 拉起宿主进程 |
| `Stop` | 卸载模块 / 结束进程 |

接线前需要先处理两件基础设施问题：

1. **统一 CRT 配置。** 目前 `Dark-Tools` 用 `staticruntime "On"`，
   而 `Dark` / `Dark-Render` / `Dark-Resources` / `Launcher` 都是 `"Off"`。
   跨 DLL 传递 STL 对象时静态与动态 CRT 混用会导致堆不匹配。
2. **给模块加统一入口。** 现在各组件导出的是类而不是模块，
   建议补一个 `extern "C" IModule* DarkCreateModule(uint32_t abiVersion)`，
   由 `IModule` 提供名称、版本、依赖列表与生命周期，
   这样启动器才有「模块」这个把手，而不是只把 DLL 路径写进配置。
