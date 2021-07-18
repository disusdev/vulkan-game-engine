-- premake5.lua

workspace "vulkan-game-engine"
    configurations { "Debug" }
    platforms { "Win64" }

    filter { "platforms:Win64" }
        system "windows"
        architecture "x86_64"

project "vulkan-game-engine"
    kind "ConsoleApp"
    language "C++"
    targetdir ".bin/%{cfg.buildcfg}"
    objdir ".obj/%{cfg.buildcfg}"
    cppdialect "C++17"

    files { "./src/**.h", "./src/**.cpp" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter {"system:windows", "action:vs*"}
        systemversion("latest")
        buildoptions {"-bigobj"}
