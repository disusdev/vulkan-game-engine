-- premake5.lua

workspace "simple-game-engine"
    configurations { "Debug" }
    platforms { "Win64" }

project "engine"
    kind "ConsoleApp"
    language "C++"
    targetdir ".bin/%{cfg.buildcfg}"
    objdir ".obj/%{cfg.buildcfg}"
    cppdialect "C++17"

    files { "./src/**.h", "./src/**.cpp" }

    filter "configurations:Debug"
        symbols "On"

    filter {"system:windows", "action:vs*"}
        systemversion("latest")
        buildoptions {"-bigobj"}
