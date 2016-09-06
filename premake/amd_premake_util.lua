-- amd_premake_util.lua
-- utility code shared by AMD build scripts

-- _ACTION is a premake global variable and for our usage will be vs2012, vs2013, etc.
-- Strip "vs" from this string to make a suffix for solution and project files.
_AMD_VS_SUFFIX = "_" .. string.gsub(_ACTION, "vs", "")

-- _ACTION is a premake global variable and for our usage will be vs2012, vs2013, etc.
-- Make an allcaps version. We use this for some directory names.
_AMD_ACTION_ALL_CAPS = string.upper(_ACTION)

-- Specify build output directory structure here: e.g. Desktop_2012\x64\DLL_Debug
_AMD_SAMPLE_DIR_LAYOUT  = "Desktop%{_AMD_VS_SUFFIX}/%{cfg.platform}/%{cfg.buildcfg}"
_AMD_LIBRARY_DIR_LAYOUT = "%{_AMD_ACTION_ALL_CAPS}/%{cfg.platform}/%{cfg.buildcfg}"

-- Some projects have a "minimal dependencies" build. Give them a different output
-- directory structure to avoid collisions with the full-dependencies version.
_AMD_SAMPLE_DIR_LAYOUT_MINIMAL  = "Desktop%{_AMD_VS_SUFFIX}/minimal/%{cfg.platform}/%{cfg.buildcfg}"
_AMD_LIBRARY_DIR_LAYOUT_MINIMAL = "%{_AMD_ACTION_ALL_CAPS}/minimal/%{cfg.platform}/%{cfg.buildcfg}"

-- Specify WindowsTargetPlatformVersion here for VS2015
_AMD_WIN_SDK_VERSION = "8.1"

-- command lines for Visual Studio build events
_AMD_COPY_WIN_8_0_SDK_REDIST_TO_BIN = "if not exist \"..\\bin\\d3dcompiler_46.dll\" if exist \"$(ProgramFiles)\\Windows Kits\\8.0\\Redist\\D3D\\x64\\d3dcompiler_46.dll\" xcopy \"$(ProgramFiles)\\Windows Kits\\8.0\\Redist\\D3D\\x64\\d3dcompiler_46.dll\" \"..\\bin\" /H /R /Y > nul"
_AMD_COPY_WIN_8_1_SDK_REDIST_TO_BIN = "if not exist \"..\\bin\\d3dcompiler_47.dll\" if exist \"$(ProgramFiles)\\Windows Kits\\8.1\\Redist\\D3D\\x64\\d3dcompiler_47.dll\" xcopy \"$(ProgramFiles)\\Windows Kits\\8.1\\Redist\\D3D\\x64\\d3dcompiler_47.dll\" \"..\\bin\" /H /R /Y > nul"
_AMD_COPY_AGS_RLS_DLL_TO_BIN = "xcopy \"..\\..\\ags_lib\\lib\\amd_ags_x64.dll\"  \"..\\bin\" /H /R /Y > nul"

-- post-build commands for samples
function amdSamplePostbuildCommands(copyAgs)
   local commands = {}
   local doCopyAgs = copyAgs or false
   -- for VS2012 and earlier, copy d3dcompiler_46.dll from the 8.0 SDK to the local bin directory
   if _ACTION <= "vs2012" then
      table.insert(commands, _AMD_COPY_WIN_8_0_SDK_REDIST_TO_BIN)
   end
   -- copy d3dcompiler_47.dll from the 8.1 SDK to the local bin directory
   table.insert(commands, _AMD_COPY_WIN_8_1_SDK_REDIST_TO_BIN)
   if doCopyAgs then
      -- copy the AGS DLLs to the local bin directory
      table.insert(commands, _AMD_COPY_AGS_RLS_DLL_TO_BIN)
   end
   return commands
end
