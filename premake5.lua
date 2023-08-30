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

   -- You can remove fmt from include directories by defining 'PNG_NO_LOGGING',
   --  which tells png to not include any sort of logging in header files
   -- defines "PNG_NO_LOGGING"
   includedirs { "include", "libs/fmt/include", }
   -- If you do not want to link with fmt,
   --  you must define 'PNG_NO_LOGGING' when building png as a static lib
   -- zlib is a required dependency since it is needed to de/inflate Images
   links { "png", "fmt", "zlib", }
   files "src/main.cpp"

   filter "toolset:gcc"
      buildoptions { "-Wall", "-Wextra", "-Wpedantic", "-Werror", }

   filter "toolset:msc"
      buildoptions { "/Wall", "/WX", }

   filter "system:linux"
      links { "pthread", "tbb", }

   filter "configurations:Debug"
      defines "PNG_DEBUG"
      symbols "On"

   filter "configurations:Release"
      optimize "Speed"
