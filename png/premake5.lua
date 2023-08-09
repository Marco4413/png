include "../libs/fmt"
include "../libs/zlib"

project "png"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"

   location "../build"
   targetdir "%{prj.location}/%{cfg.buildcfg}"

   includedirs { "../libs/fmt/include", "../libs/zlib/include", }
   links { "fmt", "zlib", }
   
   includedirs "../include"
   files { "../src/**.cpp", "../include/**.h", }
   removefiles "../src/main.cpp"

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
