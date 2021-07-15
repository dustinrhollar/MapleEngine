
#define WINDOW_CLASS_NAME "MAPLE_PROTOTYPE_WINDOW"
#include <dxgi.h>

// Heap allocated. Don't worry about size.
struct HostWnd 
{
    WND_ID             id;
    HWND               handle;
    RECT               rect;
    DWORD              style;
    DWORD              ex_style;
    // Window Settings
    u8                 is_minimized:1;
    u8                 is_fullscreen:1;
    u8                 is_running:1;
    u8                 pad0:5;
    // Window dimensions
    u32                width;
    u32                height;
    // Event layers, input callbacks
    input_layer::Layer layers[input_layer::Layer_LayerCount]; 
};

namespace win32_wnd
{
    static const u32 MIN_FREE_INDICES = 10;
    static HostWnd *g_cache = 0;
    static u8      *g_gens  = 0;
    static u32     *g_free_indices = 0;
    
    static void InitCache();
    static void FreeCache();
    
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    FORCE_INLINE MapleKey TranslateWin32Key(WPARAM key);
    
    static WND_ID Win32CreateWindow(u32 width, u32 height, const char *title, const char *window_class);
    static void Win32CreateWindow(HostWnd *window, i32 default_width, i32 default_height, TCHAR *title, TCHAR *window_class);
    
    static void Win32RegisterWindowClass();
    static void Win32ToggleFullscreen(HostWnd* window, IDXGISwapChain* pSwapChain);
    static void Win32SetWindowZorderToTopMost(HostWnd* window, bool is_top);
    static bool Win32IsKeyReleased(WND_ID id, MapleKey key);
    static bool Win32IsKeyPressed(WND_ID id, MapleKey key);
    static void Win32SetActive(WND_ID id);
    static void Win32SetCallbacks(WND_ID id, HostWndCallbacks *callbacks);
    static bool Win32MsgLoop(WND_ID id);
    static void Win32GetWindowDims(WND_ID id, u32 *width, u32 *height);
    static void* Win32GetWindowHandle(WND_ID id);
    static void Win32FreeWindow(WND_ID id);
    static bool Win32IsWindowValid(WND_ID id);
    static WND_ID Win32GenWindowId();
    
    // Callback stubs
    void HostWndResizeStub(WND_ID wnd, u32 width, u32 height) {}
    void HostWndKeyPressStub(MapleKey key) {}
    void HostWndKeyReleaseStub(MapleKey key) {}
    void HostWndFullscreenStub() {}
    
    // Event handling
    static void Win32AttachListener(WND_ID                     id, 
                                    input_layer::LayerType     layer, 
                                    input_layer::EventType     type, 
                                    input_layer::EventCallback callback, 
                                    void                      *event_args);
    static bool Win32DispatchEvent(HostWnd* wnd, input_layer::Event event);
    
};

static void 
win32_wnd::InitCache()
{
}

static void 
win32_wnd::FreeCache()
{
    arrfree(g_cache);
    arrfree(g_gens);
    arrfree(g_free_indices);
}

static WND_ID 
win32_wnd::Win32GenWindowId()
{
    WND_ID result = {};
    
    if (arrlen(g_free_indices) > MIN_FREE_INDICES)
    {
        result.idx = g_free_indices[0];
        result.gen = g_gens[result.idx];
        arrdel(g_free_indices, 0);
    }
    else
    {
        result.idx = arrlen(g_gens);
        result.gen = 0;
        
        arrput(g_gens, result.gen);
        // Put dummy window into storage
        HostWnd wnd = {}; arrput(g_cache, wnd);
    }
    
    return result;
}

static bool 
win32_wnd::Win32IsWindowValid(WND_ID id)
{
    return (id.val != INVALID_WND_ID.val) && (id.idx < arrlen(g_gens)) && (id.gen == g_gens[id.idx]);
}

static WND_ID 
win32_wnd::Win32CreateWindow(u32 width, u32 height, const char *title, const char *window_class)
{
    WND_ID result = Win32GenWindowId();
    HostWnd *window = g_cache + result.idx; 
    
    window->id = result;
    window->is_running    = false;
    window->is_fullscreen = 0;
    window->is_minimized  = 0;
    
    // Callback stubs
#if 0
    ZeroMemory(&window->callbacks, sizeof(HostWndCallbacks));
    window->callbacks.resize  = HostWndResizeStub;
    window->callbacks.press   = HostWndKeyPressStub;
    window->callbacks.release = HostWndKeyReleaseStub;
#endif
    
    Win32RegisterWindowClass();
    Win32CreateWindow(window, width, height, (TCHAR*)title, (TCHAR*)window_class);
    
    for (u32 i = 0; i < input_layer::Layer_LayerCount; ++i)
        window->layers[i].Init();
    
    return result;
}

static void 
win32_wnd::Win32FreeWindow(WND_ID id)
{
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        window->is_running = false;
        DestroyWindow(window->handle);
        window->handle = NULL;
        for (u32 i = 0; i < input_layer::Layer_LayerCount; ++i) window->layers[i].Free();
        // invalidates the id
        g_gens[id.idx]++;
    }
}

static bool
win32_wnd::Win32DispatchEvent(HostWnd* window, input_layer::Event event)
{
    for (u32 i = 0; i < input_layer::Layer_LayerCount; ++i)
        if (window->layers[i].DispatchEvent(event)) return true;
    return false;
}


static void 
win32_wnd::Win32AttachListener(WND_ID                     id, 
                               input_layer::LayerType     layer, 
                               input_layer::EventType     type, 
                               input_layer::EventCallback callback, 
                               void                      *event_args)
{
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        window->layers[layer].AddListener(type, callback, event_args);
    }
}

static void* 
win32_wnd::Win32GetWindowHandle(WND_ID id)
{
    void *result = 0;
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        result = (void*)window->handle;
    }
    return result;
}

static void 
win32_wnd::Win32GetWindowDims(WND_ID id, u32 *width, u32 *height)
{
    *width = 0;
    *height = 0;
    
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx;
        
        RECT rect;
        GetClientRect(window->handle, &rect);
        *width  = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
}

static bool 
win32_wnd::Win32MsgLoop(WND_ID id)
{
    bool result = false;
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        
        // Clear keyboard state
        //memset(window->keyboard.pressed,  0, sizeof(u8) * (u32)Key_Count);
        //memset(window->keyboard.released, 0, sizeof(u8) * (u32)Key_Count);
        
        // Process messages
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                window->is_running = false;
            }
        }
        result = window->is_running;
    }
    return result;
}

static void 
win32_wnd::Win32SetCallbacks(WND_ID id, HostWndCallbacks *callbacks)
{
#if 0
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        window->callbacks = *callbacks;
        
        if (!window->callbacks.resize)  window->callbacks.resize   = HostWndResizeStub;
        if (!window->callbacks.press)   window->callbacks.press    = HostWndKeyPressStub;
        if (!window->callbacks.release) window->callbacks.release  = HostWndKeyReleaseStub;
    }
#else
    assert(false && "Win32SetCallbacks no longer supported!");
#endif
}

static void 
win32_wnd::Win32SetActive(WND_ID id)
{
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        ShowWindow(window->handle, SW_SHOWNORMAL);
        window->is_running = true;
    }
}

static bool 
win32_wnd::Win32IsKeyPressed(WND_ID id, MapleKey key)
{
    bool result = false;
#if 0
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        result = (bool)window->keyboard.pressed[(u32)key];
    }
#else
    assert(false && "Win32IsKeyPressed no longer supported!");
#endif
    return result;
}

static bool 
win32_wnd::Win32IsKeyReleased(WND_ID id, MapleKey key)
{
    bool result = false;
#if 0
    if (Win32IsWindowValid(id))
    {
        HostWnd *window = g_cache + id.idx; 
        result = (bool)window->keyboard.released[(u32)key];
    }
#else
    assert(false && "Win32IsKeyReleased no longer supported!");
#endif
    return result;
}

static void 
win32_wnd::Win32SetWindowZorderToTopMost(HostWnd* window, bool is_top)
{
    RECT rect;
    GetWindowRect(window->handle, &rect);
    SetWindowPos(window->handle,
                 (is_top) ? HWND_TOPMOST : HWND_NOTOPMOST,
                 rect.left,
                 rect.top,
                 rect.right - rect.left,
                 rect.bottom - rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

// Convert a styled window into a fullscreen borderless window and back again.
static void 
win32_wnd::Win32ToggleFullscreen(HostWnd* window, IDXGISwapChain* pSwapChain)
{
    // WindowProc will set the fullscreen. If reverting from fullscreen,
    // this field will be FALSE
    if (!(bool)window->is_fullscreen)
    {
        // Restore the window's attributes and size.
        SetWindowLong(window->handle, GWL_STYLE, window->style);
        
        SetWindowPos(window->handle,
                     HWND_NOTOPMOST,
                     window->rect.left,
                     window->rect.top,
                     window->rect.right - window->rect.left,
                     window->rect.bottom - window->rect.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        
        ShowWindow(window->handle, SW_NORMAL);
    }
    else
    {
        // Save the old window rect so we can restore it when exiting fullscreen mode.
        GetWindowRect(window->handle, &window->rect);
        
        // Make the window borderless so that the client area can fill the screen.
        SetWindowLong(window->handle, GWL_STYLE, window->style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));
        
        RECT fullscreenWindowRect;
#if 0
        if (pSwapChain)
        {
            // Get the settings of the display on which the app's window is currently displayed
            IDXGIOutput *pOutput;
            ThrowIfFailed(pSwapChain->GetContainingOutput(&pOutput), "Failed to get containing output for swapchain");
            DXGI_OUTPUT_DESC Desc;
            ThrowIfFailed(pOutput->GetDesc(&Desc), "HostWndToggleFullscreen: Failed to get swapchain desc!");
            fullscreenWindowRect = Desc.DesktopCoordinates;
        }
        else
#endif
        {
            // Fallback to EnumDisplaySettings implementation
            LogError("Fallback to EnumDisplaySettings implementation");
            
            // Get the settings of the primary display
            DEVMODE devMode = {};
            devMode.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);
            
            fullscreenWindowRect = {
                devMode.dmPosition.x,
                devMode.dmPosition.y,
                devMode.dmPosition.x + static_cast<LONG>(devMode.dmPelsWidth),
                devMode.dmPosition.y + static_cast<LONG>(devMode.dmPelsHeight)
            };
        }
        
        SetWindowPos(window->handle,
                     HWND_TOPMOST,
                     fullscreenWindowRect.left,
                     fullscreenWindowRect.top,
                     fullscreenWindowRect.right,
                     fullscreenWindowRect.bottom,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        
        ShowWindow(window->handle, SW_MAXIMIZE);
    }
}

static void 
win32_wnd::Win32RegisterWindowClass() 
{
    WNDCLASSEX window_class;
    ZeroMemory(&window_class, sizeof(WNDCLASSEX));
    
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = GetModuleHandle(0);
    //window_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    window_class.hCursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    window_class.hIcon = (HICON)LoadImage(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    window_class.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClassEx(&window_class);
}

static void 
win32_wnd::Win32CreateWindow(HostWnd *window, i32 default_width, i32 default_height, TCHAR *title, TCHAR *wnd_class) 
{
    // Get the current screen size, so we can center our window.
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    RECT rect = { 0, 0, default_width, default_height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    LONG width = rect.right - rect.left;
    LONG height = rect.bottom - rect.top;
    LONG window_x = (screen_width - width) / 2;
    LONG window_y = (screen_height - height) / 2;
    if (window_x < 0) window_x = CW_USEDEFAULT;
    if (window_y < 0) window_y = CW_USEDEFAULT;
    if (width < 0) width = CW_USEDEFAULT;
    if (height < 0) height = CW_USEDEFAULT;
    
    window->style = WS_OVERLAPPEDWINDOW;
    window->ex_style = WS_EX_OVERLAPPEDWINDOW;
    
    window->handle = CreateWindowEx(window->ex_style,
                                    (wnd_class) ? wnd_class : WINDOW_CLASS_NAME,
                                    title,
                                    window->style,
                                    window_x,
                                    window_y,
                                    width,
                                    height,
                                    0,
                                    0,
                                    GetModuleHandle(0),
                                    window);
}

FORCE_INLINE MapleKey 
win32_wnd::TranslateWin32Key(WPARAM key)
{
    MapleKey result = Key_Unknown;
    
    if (key >= 0x41 && key <= 0x5A)
    { // Key is between A and Z
        result = (MapleKey)((u32)Key_A + (key - 0x41));
    }
    else if (key >= 0x30 && key <= 0x39)
    { // Key is between 0 and 9
        result = (MapleKey)((u32)Key_Zero + (key - 0x30));
    }
    else if (key == VK_ESCAPE)
    {
        result = Key_Escape;
    }
    else if (key == VK_RETURN)
    {
        result = Key_Return;
    }
    else if (key == VK_SPACE)
    {
        result = Key_Space;
    }
    else if (key == VK_LEFT)
    {
        result = Key_Left;
    }
    else if (key == VK_RIGHT)
    {
        result = Key_Right;
    }
    else if (key == VK_UP)
    {
        result = Key_Up;
    }
    else if (key == VK_DOWN)
    {
        result = Key_Down;
    }
    
    return result;
};

LRESULT CALLBACK 
win32_wnd::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HostWnd *window = NULL;
    if (message == WM_NCCREATE) 
    {
        LPCREATESTRUCT create_struct = (LPCREATESTRUCT)lparam;
        window = (HostWnd*)create_struct->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    }
    else 
    {
        window = (HostWnd*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (window) 
    {
        input_layer::Event event = {};
        
        // Attempt to dispatch a message to any listeners that have
        // subscribed to all events.
        event.type = input_layer::Event_AllEvents;
        event.all_events.hwnd    = (void*)hwnd;
        event.all_events.message = message;
        event.all_events.wparam  = wparam;
        event.all_events.lparam  = lparam;
        if (Win32DispatchEvent(window, event)) return 0;
        
        // Invalidate the event, and continue
        event.type = input_layer::Event_Count;
        
        // Shouldn't have to do this anymore!
        //if (Win32ImGuiWndProcHandler(window->handle, message, wparam, lparam))
        //return 0;
        
        switch (message) 
        {
            case WM_CLOSE:
            case WM_DESTROY: 
            {
                window->is_running = false;
                event.type = input_layer::Event_Close;
            } break;
            
            case WM_PAINT:
            {
                RECT rect;
                GetClientRect(hwnd, &rect);
                ValidateRect(hwnd, &rect);
                return 0;
            }
            
            case WM_SIZE:
            {
                RECT rect;
                GetClientRect(window->handle, &rect);
                window->width = rect.right - rect.left;
                window->height = rect.bottom - rect.top;
                window->is_minimized = (wparam == SIZE_MINIMIZED);
                
                event.type = input_layer::Event_Size;
                event.size.width = window->width;
                event.size.height = window->height;
            } break;
            
            case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
            {
                event.type = input_layer::Event_ButtonPress;
                
                int button = 0;
                if (message == WM_LBUTTONDOWN) { button = 0; }
                if (message == WM_RBUTTONDOWN) { button = 1; }
                if (message == WM_MBUTTONDOWN) { button = 2; }
                if (message == WM_XBUTTONDOWN) { button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? 3 : 4; }
                event.button_press.button = button;
                event.button_press.mod_ctrl  = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.button_press.mod_shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.button_press.mod_menu  = (::GetKeyState(VK_MENU) & 0x8000) != 0;
            } break;
            
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                event.type = input_layer::Event_ButtonRelease;
                
                int button = 0;
                if (message == WM_LBUTTONUP) { button = 0; }
                if (message == WM_RBUTTONUP) { button = 1; }
                if (message == WM_MBUTTONUP) { button = 2; }
                if (message == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? 3 : 4; }
                event.button_release.button = button;
                event.button_release.mod_ctrl  = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.button_release.mod_shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.button_release.mod_menu  = (::GetKeyState(VK_MENU) & 0x8000) != 0;
            } break;
            
            case WM_MOUSEWHEEL:
            {
                event.type  = input_layer::Event_MouseWheel;
                event.mouse_wheel.delta = (r32)GET_WHEEL_DELTA_WPARAM(wparam) / (r32)WHEEL_DELTA;
            } break;
            
            case WM_MOUSEHWHEEL:
            {
                event.type  = input_layer::Event_MouseHWheel;
                event.mouse_hwheel.delta = (r32)GET_WHEEL_DELTA_WPARAM(wparam) / (r32)WHEEL_DELTA;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                // TODO(Dustin): Check for modifiers?
                
                event.type = input_layer::Event_KeyPressed;
                event.key_press.mod_ctrl  = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.key_press.mod_shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.key_press.mod_menu  = (::GetKeyState(VK_MENU) & 0x8000) != 0;
                event.key_press.key = TranslateWin32Key(wparam);
                
                switch (wparam)
                {
                    case VK_RETURN:
                    {
                        if (alt)
                        {
                            case VK_F11: 
                            {
                                // NOTE(Dustin): Workaround for not having a swapchain from graphics
                                Win32ToggleFullscreen(window, NULL);
                            } break;
                        }
                    } break;
                }
            } break;
            
            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                event.type = input_layer::Event_KeyRelease;
                event.key_release.mod_ctrl  = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.key_release.mod_shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.key_release.mod_menu  = (::GetKeyState(VK_MENU) & 0x8000) != 0;
                event.key_release.key = TranslateWin32Key(wparam);
            } break;
            
            case WM_KILLFOCUS:
            {
                event.type = input_layer::Event_KillFocus;
            } break;
            
            case WM_SETCURSOR:
            {
                event.type = input_layer::Event_SetCursor;
                event.set_cursor.c = lparam;
            } break;
            
            case WM_DEVICECHANGE:
            {
                event.type = input_layer::Event_DeviceChange;
                event.device_change.c = wparam;
            } break;
            
            case WM_DISPLAYCHANGE:
            {
                event.type = input_layer::Event_DisplayChange;
            } break;
            
            case WM_MOUSEMOVE:
            {
                i32 x = GET_X_LPARAM(lparam);
                i32 y = GET_Y_LPARAM(lparam);
                
                /* Win32 is pretty braindead about the x, y position that
                                                   it returns when the mouse is off the left or top edge
                                                   of the window (due to them being unsigned). therefore,
                                                   roll the Win32's 0..2^16 pointer co-ord range to the
                                                   more amenable (and useful) 0..+/-2^15. */
                if(x & 1 << 15) x -= (1 << 16);
                if(y & 1 << 15) y -= (1 << 16);
                
                event.type = input_layer::Event_MouseMove;
                event.mouse_move.x = x;
                event.mouse_move.y = y;
            } break;
            
            case WM_CHAR:
            {
                if (wparam > 0 && wparam < 0x10000)
                {
                    event.type = input_layer::Event_Char;
                    event.wchar.c = wparam;
                }
            } break;
        }
        
        if (event.type != input_layer::Event_Count && Win32DispatchEvent(window, event)) return 0;
    }
    
    return DefWindowProc(hwnd, message, wparam, lparam);
}

WND_ID  
HostWndInit(u32 width, u32 height, const char *title, const char *window_class)
{
    return win32_wnd::Win32CreateWindow(width, height, title, window_class);
}

void  
HostWndFree(WND_ID wnd)
{
    win32_wnd::Win32FreeWindow(wnd);
}

void  
HostWndSetCallbacks(WND_ID wnd, HostWndCallbacks *callbacks)
{
    win32_wnd::Win32SetCallbacks(wnd, callbacks);
}

void  
HostWndSetActive(WND_ID wnd)
{
    win32_wnd::Win32SetActive(wnd);
}

bool  
HostWndMsgLoop(WND_ID wnd)
{
    return win32_wnd::Win32MsgLoop(wnd);;
}

bool  
HostWndIsKeyPressed(WND_ID wnd, MapleKey key)
{
    return win32_wnd::Win32IsKeyPressed(wnd, key);
}

bool  
HostWndIsKeyReleased(WND_ID wnd, MapleKey key)
{
    return win32_wnd::Win32IsKeyReleased(wnd, key);
}

void  
HostWndGetDims(WND_ID wnd, u32 *width, u32 *height)
{
    win32_wnd::Win32GetWindowDims(wnd, width, height);
}

void* 
HostWndGetHandle(WND_ID wnd)
{
    return win32_wnd::Win32GetWindowHandle(wnd);
}

void HostWndAttachListener(WND_ID                     id, 
                           input_layer::LayerType     layer, 
                           input_layer::EventType     type, 
                           input_layer::EventCallback callback, 
                           void                      *event_args)
{
    win32_wnd::Win32AttachListener(id, layer, type, callback, event_args);
}
