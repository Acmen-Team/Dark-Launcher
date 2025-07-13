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

  files
  {
    "%{prj.name}/include/**.h",
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp",
  }

  	includedirs
  {
    "Launcher/include",
    "%{IncludeDir.ImGui}",
  }

  links
  {
    "ImGui",
    "d3d11.lib",
  }

  filter "system:windows"
    systemversion "latest"

    defines
    {

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
