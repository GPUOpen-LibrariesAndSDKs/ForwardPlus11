_AMD_SAMPLE_NAME = "ForwardPlus11"

dofile ("../../premake/amd_premake_util.lua")

workspace (_AMD_SAMPLE_NAME)
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "../build"
   filename (_AMD_SAMPLE_NAME .. _AMD_VS_SUFFIX)
   startproject (_AMD_SAMPLE_NAME)

   filter "platforms:x64"
      system "Windows"
      architecture "x64"

externalproject "AMD_SDK"
   kind "StaticLib"
   language "C++"
   location "../../AMD_SDK/build"
   filename ("AMD_SDK" .. _AMD_VS_SUFFIX)
   uuid "EBB939DC-98E4-49DF-B1F1-D2E80A11F60A"

externalproject "DXUT"
   kind "StaticLib"
   language "C++"
   location "../../DXUT/Core"
   filename ("DXUT" .. _AMD_VS_SUFFIX)
   uuid "85344B7F-5AA0-4E12-A065-D1333D11F6CA"

externalproject "DXUTOpt"
   kind "StaticLib"
   language "C++"
   location "../../DXUT/Optional"
   filename ("DXUTOpt" .. _AMD_VS_SUFFIX)
   uuid "61B333C2-C4F7-4CC1-A9BF-83F6D95588EB"

project (_AMD_SAMPLE_NAME)
   kind "WindowedApp"
   language "C++"
   location "../build"
   filename (_AMD_SAMPLE_NAME .. _AMD_VS_SUFFIX)
   targetdir "../bin"
   objdir "../build/%{_AMD_SAMPLE_DIR_LAYOUT}"
   warnings "Extra"
   floatingpoint "Fast"

   -- Specify WindowsTargetPlatformVersion here for VS2015
   windowstarget (_AMD_WIN_SDK_VERSION)

   -- Copy DLLs to the local bin directory
   postbuildcommands { amdSamplePostbuildCommands() }
   postbuildmessage "Copying dependencies..."

   files { "../src/**.h", "../src/**.cpp", "../src/**.rc", "../src/**.manifest", "../src/**.hlsl" }
   includedirs { "../src/ResourceFiles" }
   links { "AMD_SDK", "DXUT", "DXUTOpt", "d3dcompiler", "dxguid", "winmm", "comctl32", "Usp10", "Shlwapi" }

   filter "configurations:Debug"
      defines { "WIN32", "_DEBUG", "DEBUG", "PROFILE", "_WINDOWS", "_WIN32_WINNT=0x0601" }
      flags { "Symbols", "FatalWarnings", "Unicode", "WinMain" }
      targetsuffix ("_Debug" .. _AMD_VS_SUFFIX)

   filter "configurations:Release"
      defines { "WIN32", "NDEBUG", "PROFILE", "_WINDOWS", "_WIN32_WINNT=0x0601" }
      flags { "LinkTimeOptimization", "Symbols", "FatalWarnings", "Unicode", "WinMain" }
      targetsuffix ("_Release" .. _AMD_VS_SUFFIX)
      optimize "On"
