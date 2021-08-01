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

project "resources"
    kind "Utility"

    targetdir ".bin/%{cfg.buildcfg}"
    objdir ".obj/%{cfg.buildcfg}"

    files { "./data/**" }

    configuration "windows"
        prebuildcommands { 'pushd .bin\nIF NOT EXIST data mklink /j "data" "../data"\npopd' }
        prebuildmessage "Create folder link..."

    filter 'files:**.frag'
        buildmessage 'Compiling %{wks.location}%{file.relpath}'
        buildcommands '"$(VULKAN_SDK)/Bin/glslangValidator.exe" -V "%{wks.location}%{file.relpath}" -o "%{file.directory}%{file.name}.spv"'
        buildoutputs "%{file.directory}%{file.name}.spv"
    
    filter 'files:**.vert'
        buildmessage 'Compiling %{wks.location}%{file.relpath}'
        buildcommands '"$(VULKAN_SDK)/Bin/glslangValidator.exe" -V "%{wks.location}%{file.relpath}" -o "%{file.directory}%{file.name}.spv"'
        buildoutputs "%{file.directory}%{file.name}.spv"

    filter {"system:windows", "action:vs*"}
        systemversion("latest")
