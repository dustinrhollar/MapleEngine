
#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <shobjidl_core.h>
#include <windowsx.h>
#include <timeapi.h>

//#include "Win32/Win32LoadModules.cpp"

//Renderer source
#include "Renderer/Renderer.cpp"


// Terrain source
#include "Terrain/Terrain.cpp"

// Editor source
#include "Editor/Editor.cpp"


#include "Win32/Win32Timer.cpp"
#include "Win32/Win32Logger.cpp"
#include "Win32/Win32File.cpp"
#include "Win32/Win32CoreUtils.cpp"
#include "Win32/Win32ThreadPool.cpp"
//#include "Win32/Win32DllEntry.cpp"
#include "Win32/Win32Window.cpp"
#include "Win32/Win32Imgui.cpp"
#include "Win32/Win32Main.cpp"

#else
#error Platform Not Supported!
#endif
