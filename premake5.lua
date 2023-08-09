term.pushColor(term.yellow)
print("WARNING: IF YOU SEE THIS MESSAGE AND YOU ARE TRYING TO INCLUDE THE PNG PROJECT, YOU ARE ACTUALLY INCLUDING THE WORKSPACE FILE.")
print("      -> If so, make sure to include the png folder instead.")
term.popColor()

workspace "png"
   architecture "x64"
   configurations { "Debug", "Release", }
   startproject "png-dev"

include "png"

project "png-dev"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"

   location "build"
   targetdir "%{prj.location}/%{cfg.buildcfg}"

   includedirs { "libs/fmt/include", "libs/zlib/include", }
   links { "fmt", "zlib", }
   
   includedirs "include"
   files { "src/**.cpp", "include/**.h", }

   filter "toolset:gcc"
      buildoptions { "-Wall", "-Wextra", }

   filter "toolset:msc"
      buildoptions "/W3"

   filter "system:linux"
      links { "pthread", "tbb" }

   filter "configurations:Debug"
      defines "PNG_DEBUG"
      symbols "On"

   filter "configurations:Release"
      optimize "Speed"
