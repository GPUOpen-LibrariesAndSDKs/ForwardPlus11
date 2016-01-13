-- amd_premake_util.lua
-- utility code shared by AMD build scripts

-- _ACTION is a premake global variable and for our usage will be vs2012, vs2013, etc.
-- Strip "vs" from this string to make a suffix for solution and project files.
_AMD_VS_SUFFIX = "_" .. string.gsub(_ACTION, "vs", "")

-- Specify build output directory structure here: e.g. Desktop_2012\x64\DLL_Debug
_AMD_SAMPLE_DIR_LAYOUT  = "Desktop%{_AMD_VS_SUFFIX}/%{cfg.platform}/%{cfg.buildcfg}"

-- Specify WindowsTargetPlatformVersion here for VS2015
_AMD_WIN_SDK_VERSION = "10.0.10240.0"

-- command lines for Visual Studio build events
_AMD_COPY_WIN_8_0_SDK_REDIST_TO_BIN = "if not exist \"..\\bin\\d3dcompiler_46.dll\" if exist \"$(ProgramFiles)\\Windows Kits\\8.0\\Redist\\D3D\\x64\\d3dcompiler_46.dll\" xcopy \"$(ProgramFiles)\\Windows Kits\\8.0\\Redist\\D3D\\x64\\d3dcompiler_46.dll\" \"..\\bin\" /H /R /Y > nul"
_AMD_COPY_WIN_8_1_SDK_REDIST_TO_BIN = "if not exist \"..\\bin\\d3dcompiler_47.dll\" if exist \"$(ProgramFiles)\\Windows Kits\\8.1\\Redist\\D3D\\x64\\d3dcompiler_47.dll\" xcopy \"$(ProgramFiles)\\Windows Kits\\8.1\\Redist\\D3D\\x64\\d3dcompiler_47.dll\" \"..\\bin\" /H /R /Y > nul"
_AMD_COPY_AGS_DBG_DLL_TO_BIN = "if not exist \"..\\bin\\amd_ags_x64d.dll\" xcopy \"..\\..\\ags_lib\\lib\\amd_ags_x64d.dll\" \"..\\bin\" /H /R /Y > nul"
_AMD_COPY_AGS_RLS_DLL_TO_BIN = "if not exist \"..\\bin\\amd_ags_x64.dll\"  xcopy \"..\\..\\ags_lib\\lib\\amd_ags_x64.dll\"  \"..\\bin\" /H /R /Y > nul"

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
      table.insert(commands, _AMD_COPY_AGS_DBG_DLL_TO_BIN)
      table.insert(commands, _AMD_COPY_AGS_RLS_DLL_TO_BIN)
   end
   return commands
end
