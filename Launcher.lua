workspace "Launcher"
  architecture "x86_64"
  startproject "Launcher"

  configurations
  {
    "Debug",
    "Release",
    "Dist"
  }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder(solution directory)
IncludeDir = {}
IncludeDir["ImGui"] = "Launcher/vendor/imgui"
-- IncludeDir["Glad"] = "Dark/vendor/glad/include"

group "Dependencies"
  include "Launcher/vendor/ImGui"
group ""

project "Launcher"
  location "Launcher"
  kind "ConsoleApp"
  language "C++"
  cppdialect "C++20"
  staticruntime "On"

  targetdir ("bin/" .. outputdir .. "/%{prj.name}")
  objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
  debugdir ("bin/" .. outputdir .. "/%{prj.name}")

  -- 界面文案是 UTF-8 中英双语，必须显式告诉 MSVC 源码与执行字符集都是 UTF-8，
  -- 否则中文字符串会按系统代码页解释，图标码点也会错乱。
  buildoptions { "/utf-8" }

  files
  {
    "%{prj.name}/include/**.h",
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp",
  }

  	includedirs
  {
    "Launcher/include",
    -- 源码内部一律用 "UI/xxx.h"、"Platform/xxx.h" 这样的路径互相引用，
    -- 所以 src 根目录必须在搜索路径里。
    "Launcher/src",
    "%{IncludeDir.ImGui}",
  }

  links
  {
    "ImGui",
    "d3d11.lib",
    "windowscodecs.lib",   -- WIC：解码 PNG 品牌图标
    "ole32.lib",           -- CoInitializeEx / CoCreateInstance
  }

  filter "system:windows"
    systemversion "latest"

    defines
    {

    }

    -- 字体等运行期资源跟着可执行文件走，启动器才能独立运行。
    postbuildcommands
    {
      ("xcopy /Q /E /Y /I \"Content\" \"../bin/" .. outputdir .. "/%{prj.name}/Content\"")
    }

  filter "configurations:Debug"
    defines "DK_DEBUG"
    runtime "Debug"
    symbols "On"

  filter "configurations:Release"
    defines "DK_RELEASE"
    runtime "Release"
    optimize "On"

  filter "configurations:Dist"
    defines "DK_Dist"
    runtime "Release"
    optimize "On"
