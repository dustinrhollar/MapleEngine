
// Defined in HostWindow.h
using Win32ImGuiViewportData = PlatformViewportData;

struct Win32ImGuiData
{
    WND_ID               Wnd = INVALID_WND_ID;
    INT64                Time = 0;
    INT64                TicksPerSecond = 0;
    ImGuiMouseCursor     LastMouseCursor = ImGuiMouseCursor_COUNT;
    bool                 HasGamepad = false;
    bool                 WantUpdateHasGamepad = true;
    bool                 WantUpdateMonitors = true;
} static g_imgui_data = {};

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

#define _IsWindowsVistaOrGreater()   _IsWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0) // _WIN32_WINNT_VISTA
#define _IsWindows8OrGreater()       _IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _WIN32_WINNT_WIN8
#define _IsWindows8Point1OrGreater() _IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0) // _WIN32_WINNT_WINBLUE
#define _IsWindows10OrGreater()      _IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0) // _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10
typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;

typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

static void Win32ImGuiInit(WND_ID wnd);
static void Win32ImGuiInitPlatformInterface();
static void Win32ImGuiCreateWindow(ImGuiViewport* viewport);
static BOOL CALLBACK Win32ImGuiUpdateMonitors_EnumFunc(HMONITOR monitor, HDC, LPRECT, LPARAM);
static void Win32ImGuiUpdateMonitors();
static BOOL _IsWindowsVersionOrGreater(WORD major, WORD minor, WORD);
static void Win32ImGuiGetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style);
static void Win32ImGuiDestroyWindow(ImGuiViewport* viewport);
static void Win32ImGuiShowWindow(ImGuiViewport* viewport);
static void Win32ImGuiUpdateWindow(ImGuiViewport* viewport);
static ImVec2 Win32ImGuiGetWindowPos(ImGuiViewport* viewport);
static void Win32ImGuiSetWindowPos(ImGuiViewport* viewport, ImVec2 pos);
static ImVec2 Win32ImGuiGetWindowSize(ImGuiViewport* viewport);
static void Win32ImGuiSetWindowSize(ImGuiViewport* viewport, ImVec2 size);
static void Win32ImGuiSetWindowFocus(ImGuiViewport* viewport);
static bool Win32ImGuiGetWindowFocus(ImGuiViewport* viewport);
static bool Win32ImGuiGetWindowMinimized(ImGuiViewport* viewport);
static void Win32ImGuiSetWindowTitle(ImGuiViewport* viewport, const char* title);
static void Win32ImGuiSetWindowAlpha(ImGuiViewport* viewport, float alpha);
static float Win32ImGuiGetWindowDpiScale(ImGuiViewport* viewport);
static void Win32ImGuiOnChangedViewport(ImGuiViewport* viewport);
static void Win23ImGuiGetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style);
static float Win32ImGuiGetDpiScaleForMonitor(void* monitor);
static float Win32ImGuiGetDpiScaleForHwnd(void* hwnd);

static void Win32ImGuiNewFrame();
static void Win32ImGuiUpdateMousePos();
static bool Win32ImGuiUpdateMouseCursor();

bool Win32ImGuiWndProcHandler(input_layer::Event event, void *args) ;
IMGUI_IMPL_API LRESULT Win32ImGuiWndProcHandler_PlatformWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void
Win32ImGuiInit(WND_ID wnd)
{
    void *hwnd =  HostWndGetHandle(wnd);
    
    if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&g_imgui_data.TicksPerSecond))
        return;
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_imgui_data.Time))
        return;
    
    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformUserData = (void*)&g_imgui_data;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "imgui_impl_win32";
    
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    g_imgui_data.Wnd = wnd;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = hwnd;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        Win32ImGuiInitPlatformInterface();
    
    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
}

static void 
Win32ImGuiInitPlatformInterface()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Win32ImGuiWndProcHandler_PlatformWindow;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = ::GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("ImGui Platform");
    wcex.hIconSm = NULL;
    ::RegisterClassEx(&wcex);
    
    Win32ImGuiUpdateMonitors();
    
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow       = Win32ImGuiCreateWindow;
    platform_io.Platform_DestroyWindow      = Win32ImGuiDestroyWindow;
    platform_io.Platform_ShowWindow         = Win32ImGuiShowWindow;
    platform_io.Platform_SetWindowPos       = Win32ImGuiSetWindowPos;
    platform_io.Platform_GetWindowPos       = Win32ImGuiGetWindowPos;
    platform_io.Platform_SetWindowSize      = Win32ImGuiSetWindowSize;
    platform_io.Platform_GetWindowSize      = Win32ImGuiGetWindowSize;
    platform_io.Platform_SetWindowFocus     = Win32ImGuiSetWindowFocus;
    platform_io.Platform_GetWindowFocus     = Win32ImGuiGetWindowFocus;
    platform_io.Platform_GetWindowMinimized = Win32ImGuiGetWindowMinimized;
    platform_io.Platform_SetWindowTitle     = Win32ImGuiSetWindowTitle;
    platform_io.Platform_SetWindowAlpha     = Win32ImGuiSetWindowAlpha;
    platform_io.Platform_UpdateWindow       = Win32ImGuiUpdateWindow;
    platform_io.Platform_GetWindowDpiScale  = Win32ImGuiGetWindowDpiScale; // FIXME-DPI
    platform_io.Platform_OnChangedViewport  = Win32ImGuiOnChangedViewport; // FIXME-DPI
#if HAS_WIN32_IME
    platform_io.Platform_SetImeInputPos     = ImGui_ImplWin32_SetImeInputPos;
#endif
    
    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    Win32ImGuiViewportData* data = IM_NEW(Win32ImGuiViewportData)();
    data->Wnd = g_imgui_data.Wnd;
    data->HwndOwned = false;
    main_viewport->PlatformUserData = data;
    main_viewport->PlatformHandle = HostWndGetHandle(g_imgui_data.Wnd);
}

static BOOL 
_IsWindowsVersionOrGreater(WORD major, WORD minor, WORD)
{
    typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
    static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = NULL;
	if (RtlVerifyVersionInfoFn == NULL)
		if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll"))
        RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
    
	RTL_OSVERSIONINFOEXW versionInfo = { };
	ULONGLONG conditionMask = 0;
	versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	versionInfo.dwMajorVersion = major;
	versionInfo.dwMinorVersion = minor;
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
}

static void 
Win32ImGuiNewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    Win32ImGuiData* bd = &g_imgui_data;
    HWND hWnd = (HWND)HostWndGetHandle(g_imgui_data.Wnd);
    
    // Setup display size (every frame to accommodate for window resizing)
    RECT rect = { 0, 0, 0, 0 };
    ::GetClientRect(hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    if (bd->WantUpdateMonitors)
        Win32ImGuiUpdateMonitors();
    
    // Setup time step
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;
    
    // Read keyboard modifiers inputs
    io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.
    
    // Update OS mouse position
    Win32ImGuiUpdateMousePos();
    
    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (bd->LastMouseCursor != mouse_cursor)
    {
        bd->LastMouseCursor = mouse_cursor;
        Win32ImGuiUpdateMouseCursor();
    }
    
    // Update game controllers (if enabled and available)
    //Win32ImGuiUpdateGamepads();
}

// This code supports multi-viewports (multiple OS Windows mapped into different Dear ImGui viewports)
// Because of that, it is a little more complicated than your typical single-viewport binding code!
static void 
Win32ImGuiUpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();
    Win32ImGuiData* bd = &g_imgui_data;
    IM_ASSERT(bd->Wnd != INVALID_WND_ID);
    HWND hWnd = (HWND)HostWndGetHandle(g_imgui_data.Wnd);
    
    
    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    // (When multi-viewports are enabled, all imgui positions are same as OS positions)
    if (io.WantSetMousePos)
    {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
            ::ClientToScreen(hWnd, &pos);
        ::SetCursorPos(pos.x, pos.y);
    }
    
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseHoveredViewport = 0;
    
    // Set imgui mouse position
    POINT mouse_screen_pos;
    if (!::GetCursorPos(&mouse_screen_pos))
        return;
    if (HWND focused_hwnd = ::GetForegroundWindow())
    {
        if (::IsChild(focused_hwnd, hWnd))
            focused_hwnd = hWnd;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            // This is the position you can get with GetCursorPos(). In theory adding viewport->Pos is also the reverse operation of doing ScreenToClient().
            if (ImGui::FindViewportByPlatformHandle((void*)focused_hwnd) != NULL)
                io.MousePos = ImVec2((float)mouse_screen_pos.x, (float)mouse_screen_pos.y);
        }
        else
        {
            // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window.)
            // This is the position you can get with GetCursorPos() + ScreenToClient() or from WM_MOUSEMOVE.
            if (focused_hwnd == hWnd)
            {
                POINT mouse_client_pos = mouse_screen_pos;
                ::ScreenToClient(focused_hwnd, &mouse_client_pos);
                io.MousePos = ImVec2((float)mouse_client_pos.x, (float)mouse_client_pos.y);
            }
        }
    }
    
    // (Optional) When using multiple viewports: set io.MouseHoveredViewport to the viewport the OS mouse cursor is hovering.
    // Important: this information is not easy to provide and many high-level windowing library won't be able to provide it correctly, because
    // - This is _ignoring_ viewports with the ImGuiViewportFlags_NoInputs flag (pass-through windows).
    // - This is _regardless_ of whether another viewport is focused or being dragged from.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, imgui will ignore this field and infer the information by relying on the
    // rectangles and last focused time of every viewports it knows about. It will be unaware of foreign windows that may be sitting between or over your windows.
    if (HWND hovered_hwnd = ::WindowFromPoint(mouse_screen_pos))
        if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hovered_hwnd))
        if ((viewport->Flags & ImGuiViewportFlags_NoInputs) == 0) // FIXME: We still get our NoInputs window with WM_NCHITTEST/HTTRANSPARENT code when decorated?
        io.MouseHoveredViewport = viewport->ID;
}

static bool 
Win32ImGuiUpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;
    
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
            case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
            case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
            case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
            case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
            case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
            case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
            case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
            case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
            case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

static void 
Win23ImGuiGetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style)
{
    if (flags & ImGuiViewportFlags_NoDecoration)
        *out_style = WS_POPUP;
    else
        *out_style = WS_OVERLAPPEDWINDOW;
    
    if (flags & ImGuiViewportFlags_NoTaskBarIcon)
        *out_ex_style = WS_EX_TOOLWINDOW;
    else
        *out_ex_style = WS_EX_APPWINDOW;
    
    if (flags & ImGuiViewportFlags_TopMost)
        *out_ex_style |= WS_EX_TOPMOST;
}

static BOOL CALLBACK 
Win32ImGuiUpdateMonitors_EnumFunc(HMONITOR monitor, HDC, LPRECT, LPARAM)
{
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    if (!::GetMonitorInfo(monitor, &info))
        return TRUE;
    ImGuiPlatformMonitor imgui_monitor;
    imgui_monitor.MainPos = ImVec2((float)info.rcMonitor.left, (float)info.rcMonitor.top);
    imgui_monitor.MainSize = ImVec2((float)(info.rcMonitor.right - info.rcMonitor.left), (float)(info.rcMonitor.bottom - info.rcMonitor.top));
    imgui_monitor.WorkPos = ImVec2((float)info.rcWork.left, (float)info.rcWork.top);
    imgui_monitor.WorkSize = ImVec2((float)(info.rcWork.right - info.rcWork.left), (float)(info.rcWork.bottom - info.rcWork.top));
    imgui_monitor.DpiScale = Win32ImGuiGetDpiScaleForMonitor(monitor);
    ImGuiPlatformIO& io = ImGui::GetPlatformIO();
    if (info.dwFlags & MONITORINFOF_PRIMARY)
        io.Monitors.push_front(imgui_monitor);
    else
        io.Monitors.push_back(imgui_monitor);
    return TRUE;
}

static void 
Win32ImGuiUpdateMonitors()
{
    ImGui::GetPlatformIO().Monitors.resize(0);
    ::EnumDisplayMonitors(NULL, NULL, Win32ImGuiUpdateMonitors_EnumFunc, 0);
    g_imgui_data.WantUpdateMonitors = false;
}

static float 
Win32ImGuiGetDpiScaleForMonitor(void* monitor)
{
    UINT xdpi = 96, ydpi = 96;
    if (_IsWindows8Point1OrGreater())
    {
		static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
		static PFN_GetDpiForMonitor GetDpiForMonitorFn = NULL;
		if (GetDpiForMonitorFn == NULL && shcore_dll != NULL)
            GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor");
		if (GetDpiForMonitorFn != NULL)
		{
			GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
            IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
			return xdpi / 96.0f;
		}
    }
#ifndef NOGDI
    const HDC dc = ::GetDC(NULL);
    xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
    ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
    IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
    ::ReleaseDC(NULL, dc);
#endif
    return xdpi / 96.0f;
}

static void 
Win32ImGuiCreateWindow(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = IM_NEW(Win32ImGuiViewportData)();
    viewport->PlatformUserData = data;
    RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
    
    data->Wnd = HostWndInit(rect.right - rect.left, rect.bottom - rect.top, "Untitled", "ImGui Platform");
    data->HwndOwned = true;
    viewport->PlatformRequestResize = false;
    viewport->PlatformHandle = viewport->PlatformHandleRaw = HostWndGetHandle(data->Wnd);
    Win32ImGuiUpdateWindow(viewport);
}

static void 
Win32ImGuiGetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style)
{
    if (flags & ImGuiViewportFlags_NoDecoration)
        *out_style = WS_POPUP;
    else
        *out_style = WS_OVERLAPPEDWINDOW;
    
    if (flags & ImGuiViewportFlags_NoTaskBarIcon)
        *out_ex_style = WS_EX_TOOLWINDOW;
    else
        *out_ex_style = WS_EX_APPWINDOW;
    
    if (flags & ImGuiViewportFlags_TopMost)
        *out_ex_style |= WS_EX_TOPMOST;
}

static void 
Win32ImGuiDestroyWindow(ImGuiViewport* viewport)
{
    if (Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData)
    {
        HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
        if (::GetCapture() == Hwnd)
        {
            // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
            ::ReleaseCapture();
            ::SetCapture((HWND)HostWndGetHandle(g_imgui_data.Wnd));
        }
        HostWndFree(data->Wnd);
        data->Wnd = INVALID_WND_ID;
        IM_DELETE(data);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

static void 
Win32ImGuiShowWindow(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        ::ShowWindow(Hwnd, SW_SHOWNA);
    else
        ::ShowWindow(Hwnd, SW_SHOW);
}



static void 
Win32ImGuiUpdateWindow(ImGuiViewport* viewport)
{
    // (Optional) Update Win32 style if it changed _after_ creation.
    // Generally they won't change unless configuration flags are changed, but advanced uses (such as manually rewriting viewport flags) make this useful.
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    DWORD new_style;
    DWORD new_ex_style;
    Win23ImGuiGetWin32StyleFromViewportFlags(viewport->Flags, &new_style, &new_ex_style);
    
    // Only reapply the flags that have been changed from our point of view (as other flags are being modified by Windows)
    if (data->DwStyle != new_style || data->DwExStyle != new_ex_style)
    {
        // (Optional) Update TopMost state if it changed _after_ creation
        bool top_most_changed = (data->DwExStyle & WS_EX_TOPMOST) != (new_ex_style & WS_EX_TOPMOST);
        HWND insert_after = top_most_changed ? ((viewport->Flags & ImGuiViewportFlags_TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST) : 0;
        UINT swp_flag = top_most_changed ? 0 : SWP_NOZORDER;
        
        // Apply flags and position (since it is affected by flags)
        data->DwStyle = new_style;
        data->DwExStyle = new_ex_style;
        ::SetWindowLong(Hwnd, GWL_STYLE, data->DwStyle);
        ::SetWindowLong(Hwnd, GWL_EXSTYLE, data->DwExStyle);
        RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
        ::AdjustWindowRectEx(&rect, data->DwStyle, FALSE, data->DwExStyle); // Client to Screen
        ::SetWindowPos(Hwnd, insert_after, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swp_flag | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        ::ShowWindow(Hwnd, SW_SHOWNA); // This is necessary when we alter the style
        viewport->PlatformRequestMove = viewport->PlatformRequestResize = true;
    }
}

static ImVec2 
Win32ImGuiGetWindowPos(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    POINT pos = { 0, 0 };
    ::ClientToScreen(Hwnd, &pos);
    return ImVec2((float)pos.x, (float)pos.y);
}

static void 
Win32ImGuiSetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    RECT rect = { (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
    ::AdjustWindowRectEx(&rect, data->DwStyle, FALSE, data->DwExStyle);
    ::SetWindowPos(Hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

static ImVec2 
Win32ImGuiGetWindowSize(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    RECT rect;
    ::GetClientRect(Hwnd, &rect);
    return ImVec2(float(rect.right - rect.left), float(rect.bottom - rect.top));
}

static void 
Win32ImGuiSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
    ::AdjustWindowRectEx(&rect, data->DwStyle, FALSE, data->DwExStyle); // Client to Screen
    ::SetWindowPos(Hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

static void 
Win32ImGuiSetWindowFocus(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    ::BringWindowToTop(Hwnd);
    ::SetForegroundWindow(Hwnd);
    ::SetFocus(Hwnd);
}

static bool 
Win32ImGuiGetWindowFocus(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    return ::GetForegroundWindow() == Hwnd;
}

static bool 
Win32ImGuiGetWindowMinimized(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    return ::IsIconic(Hwnd) != 0;
}

static void 
Win32ImGuiSetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    // ::SetWindowTextA() doesn't properly handle UTF-8 so we explicitely convert our string.
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    int n = ::MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    ImVector<wchar_t> title_w;
    title_w.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w.Data, n);
    ::SetWindowTextW(Hwnd, title_w.Data);
}

static float 
Win32ImGuiGetDpiScaleForHwnd(void* hwnd)
{
    HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
    return Win32ImGuiGetDpiScaleForMonitor(monitor);
}

static void 
Win32ImGuiSetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(alpha >= 0.0f && alpha <= 1.0f);
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    
    if (alpha < 1.0f)
    {
        DWORD style = ::GetWindowLongW(Hwnd, GWL_EXSTYLE) | WS_EX_LAYERED;
        ::SetWindowLongW(Hwnd, GWL_EXSTYLE, style);
        ::SetLayeredWindowAttributes(Hwnd, 0, (BYTE)(255 * alpha), LWA_ALPHA);
    }
    else
    {
        DWORD style = ::GetWindowLongW(Hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED;
        ::SetWindowLongW(Hwnd, GWL_EXSTYLE, style);
    }
}

static float 
Win32ImGuiGetWindowDpiScale(ImGuiViewport* viewport)
{
    Win32ImGuiViewportData* data = (Win32ImGuiViewportData*)viewport->PlatformUserData;
    IM_ASSERT(data->Wnd != INVALID_WND_ID);
    HWND Hwnd = (HWND)HostWndGetHandle(data->Wnd);
    return Win32ImGuiGetDpiScaleForHwnd(Hwnd);
}

// FIXME-DPI: Testing DPI related ideas
static void 
Win32ImGuiOnChangedViewport(ImGuiViewport* viewport)
{
    (void)viewport;
#if 0
    ImGuiStyle default_style;
    //default_style.WindowPadding = ImVec2(0, 0);
    //default_style.WindowBorderSize = 0.0f;
    //default_style.ItemSpacing.y = 3.0f;
    //default_style.FramePadding = ImVec2(0, 0);
    default_style.ScaleAllSizes(viewport->DpiScale);
    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
#endif
}

// Win32 message handler (process Win32 mouse/keyboard inputs, etc.)
// Call from your application's message handler.
// When implementing your own backend, you can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if Dear ImGui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to Dear ImGui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
bool Win32ImGuiWndProcHandler(input_layer::Event event, void *args) 
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;
    
    assert(event.type == input_layer::Event_AllEvents);
    
    HWND hwnd     = (HWND)event.all_events.hwnd;
    UINT msg      = (UINT)event.all_events.message;
    WPARAM wParam = (WPARAM)event.all_events.wparam;
    LPARAM lParam = (LPARAM)event.all_events.lparam;
    
    ImGuiIO& io = ImGui::GetIO();
    Win32ImGuiData* bd = &g_imgui_data;
    
    switch (msg)
    {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
            if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
            if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
                ::SetCapture(hwnd);
            io.MouseDown[button] = true;
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP) { button = 0; }
            if (msg == WM_RBUTTONUP) { button = 1; }
            if (msg == WM_MBUTTONUP) { button = 2; }
            if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            io.MouseDown[button] = false;
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
                ::ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
        case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
        return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
        return 0;
        case WM_KILLFOCUS:
        memset(io.KeysDown, 0, sizeof(io.KeysDown));
        return 0;
        case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacterUTF16((unsigned short)wParam);
        return 0;
        case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && Win32ImGuiUpdateMouseCursor())
            return 1;
        return 0;
        case WM_DEVICECHANGE:
        if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            bd->WantUpdateHasGamepad = true;
        return 0;
        case WM_DISPLAYCHANGE:
        bd->WantUpdateMonitors = true;
        return 0;
    }
    return 0;
}

IMGUI_IMPL_API LRESULT Win32ImGuiWndProcHandler_PlatformWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Attempt to dispatch a message to the primary ImGui proc handler
    input_layer::Event event = {};
    event.type = input_layer::Event_AllEvents;
    event.all_events.hwnd    = (void*)hwnd;
    event.all_events.message = msg;
    event.all_events.wparam  = wParam;
    event.all_events.lparam  = lParam;
    if (Win32ImGuiWndProcHandler(event, 0))
        return true;
    
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hwnd))
    {
        switch (msg)
        {
            case WM_CLOSE:
            viewport->PlatformRequestClose = true;
            return 0;
            case WM_MOVE:
            viewport->PlatformRequestMove = true;
            break;
            case WM_SIZE:
            viewport->PlatformRequestResize = true;
            break;
            case WM_MOUSEACTIVATE:
            if (viewport->Flags & ImGuiViewportFlags_NoFocusOnClick)
                return MA_NOACTIVATE;
            break;
            case WM_NCHITTEST:
            // Let mouse pass-through the window. This will allow the backend to set io.MouseHoveredViewport properly (which is OPTIONAL).
            // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
            // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
            // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
            if (viewport->Flags & ImGuiViewportFlags_NoInputs)
                return HTTRANSPARENT;
            break;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
