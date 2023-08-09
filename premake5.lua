workspace "png"
   architecture "x64"
   configurations { "Debug", "Release", }
   startproject "png-dev"

include "libs/fmt"
include "libs/zlib"

local function CommonProjectSetup()
   language "C++"
   cppdialect "C++20"

   location "build"
   targetdir "%{prj.location}/%{cfg.buildcfg}"

   includedirs { "libs/fmt/include", "libs/zlib/include", }
   links { "fmt", "zlib", }
   
   includedirs { "include", "libs/include", }
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
   
   filter {}
end

project "png"
   kind "StaticLib"
   CommonProjectSetup()
   removefiles "src/main.cpp"

project "png-dev"
   kind "ConsoleApp"
   CommonProjectSetup()

