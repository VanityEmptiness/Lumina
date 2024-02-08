project "lumina_engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "src/**.h", "src/**.cpp", "vendors/**.h", "vendors/**.cpp" }

   includedirs
   {
      "src",
      "vendors"
   }

   libdirs
   {
      "../vendors_bin",
   }

   links
   {
      "d3d11.lib",
      "glfw3.lib"
   }

   defines
   {
      "LUMINA_WIN32_PLATFORM"
   }

   characterset ("MBCS")

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"