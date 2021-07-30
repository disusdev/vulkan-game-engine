-- premake5.lua

workspace "simple-game-engine"
    configurations { "Debug" }
    platforms { "Win64" }

project "engine"
    kind "ConsoleApp"
    
    language "C++"
    cppdialect "C++17"

    targetdir ".bin/%{cfg.buildcfg}"
    objdir ".obj/%{cfg.buildcfg}"

    files { "./src/**.h", "./src/**.cpp" }

    links { "$(VULKAN_SDK)/lib/vulkan-1.lib" }
    includedirs { "$(VULKAN_SDK)/include" }

    filter "configurations:Debug"
        symbols "On"

    filter {"system:windows", "action:vs*"}
        systemversion("latest")
        buildoptions {"-bigobj"}
