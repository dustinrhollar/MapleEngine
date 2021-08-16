
#include "../../ext/imgui/imgui_internal.h"
#include "ViewportCamera.cpp"
//#include "ExperimentalEditor.cpp"

// TODO(Dustin): 
// - Allow for Camera Viewports to be used as separate "ImGUI Viewports"

#if 0

#endif


namespace editor
{
    //-----------------------------------------------------------------------------------//
    // Material Icons
    
    enum IconType 
    {
        Icon_Directory,
        Icon_File,
        
        Icon_LeftArrow,
        Icon_RightArrow,
        Icon_UpArrow,
        
        Icon_Count,
        Icon_Unknown = Icon_Count,
    };
    
    TEXTURE_ID g_icons[Icon_Count];
    
    //-----------------------------------------------------------------------------------//
    // General Input Tracking
    
    enum MouseMask
    {
        Button_0 = BIT(0),
        Button_1 = BIT(1),
        Button_2 = BIT(2),
        Button_3 = BIT(3),
        Button_4 = BIT(4),
    };
    
    // Track the pressed buttons the mask. Will only be set if ImGui or another viewport
    // does not consume the button press event.
    static u32                g_mouse_button_mask = 0;
    
    //-----------------------------------------------------------------------------------//
    // Main Viewport data
    
    static v2                 g_viewport_bounds[2];
    static v2                 g_viewport_size = V2_ZERO;
    static bool               g_viewport_focused = false;
    static bool               g_viewport_hovered = false;
    static RenderTarget       g_rt_viewport;
    static TEXTURE_ID         g_color_texture = INVALID_TEXTURE_ID;
    static TEXTURE_ID         g_depth_texture = INVALID_TEXTURE_ID;
    static ViewportCamera     g_main_camera;
    static ImGuiID            g_main_viewport_id;
    
    //-----------------------------------------------------------------------------------//
    // Terrain Gen Viewport Data
    
    //-----------------------------------------------------------------------------------//
    // Options
    
    static bool               g_show_imgui_metrics = false;
    static bool               g_show_imgui_demo    = false;
    
    //-----------------------------------------------------------------------------------//
    // Function Pre-decs
    
    // Editor Setup
    static void Initialize();
    static void Shutdown();
    
    static void Render();
    
    // Dockspace windows
    static void DrawMainWindow();
    static void DrawTerrainWindow();
    
    // Window Set #1
    static void DrawMainViewport();
    static void DrawContentPanel();
    static void DrawSettingsPanel();
    
    // Window Set #2
    static void DrawTerrainGraph();
    static void DrawTerrainViewport();
    static void DrawTerrainSettings();
    
    // MainViewport callbacks
    static void WindowRegisterCallbacks(WND_ID wnd);
    static bool ButtonPressCallback(input_layer::Event event, void *args);
    static bool ButtonReleaseCallback(input_layer::Event event, void *args);
    static bool KeyPressCallback(input_layer::Event event, void *args);
    static bool KeyReleaseCallback(input_layer::Event event, void *args);
    static bool CloseCallback(input_layer::Event event, void *args);
    
    struct WindowInterface
    {
        Str _name;
        virtual void OnInit(const char *name) {}
        virtual void OnClose() {}
        virtual void OnRender() {}
        /* Keyboard/Mouse handling for windows with a viewport */
        virtual bool KeyPressCallback(input_layer::Event event, void *args)      { return false; };
        virtual bool KeyReleaseCallback(input_layer::Event event, void *args)    { return false; };
        virtual bool ButtonPressCallback(input_layer::Event event, void *args)   { return false; };
        virtual bool ButtonReleaseCallback(input_layer::Event event, void *args) { return false; };
    };
    
    // TODO(Dustin): Don't include the windows here...
    // NOTE(Dustin): MainViewport references g_icons... 
#include "Panel/MainViewportWindow.cpp"
#include "Panel/TerrainGenWindow.cpp"
#include "Panel/DemoWindow.cpp"
#include "Panel/MetricsWindow.cpp"
    
    WindowInterface **window_list = 0;
    i32               selected_window = 0;
    
    
#define DEMO_WND_NAME   "Demo Window"
#define METRICS_WND_NAME "Metrics Window"
    
};

static bool 
editor::KeyPressCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_main_camera.OnKeyPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
editor::KeyReleaseCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_main_camera.OnKeyRelease((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
editor::ButtonPressCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused && g_viewport_hovered)
    {
        g_main_camera.OnMouseButtonPress(event.button_press.button);
    }
    BIT32_TOGGLE_1(g_mouse_button_mask, event.button_press.button);
    return false;
}

static bool 
editor::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    g_main_camera.OnMouseButtonRelease(event.button_press.button);
    BIT32_TOGGLE_0(g_mouse_button_mask, event.button_press.button);
    return false;
}

static bool 
editor::CloseCallback(input_layer::Event event, void *args)
{
    LogDebug("Window has been closed!");
    return false;
}

#include <new>
static void editor::Initialize()
{
    CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = command_queue->GetCommandList();
    
    //-----------------------------------------------------------------------------------//
    // Load Material icons
    
    g_icons[Icon_File]       = command_list->LoadTextureFromFile("textures/ui_icons/file.png");
    g_icons[Icon_Directory]  = command_list->LoadTextureFromFile("textures/ui_icons/directory.png");
    
    g_icons[Icon_LeftArrow]  = command_list->LoadTextureFromFile("textures/ui_icons/left_arrow.png");
    g_icons[Icon_RightArrow] = command_list->LoadTextureFromFile("textures/ui_icons/right_arrow.png");
    g_icons[Icon_UpArrow]    = command_list->LoadTextureFromFile("textures/ui_icons/up_arrow.png");
    
    command_queue->ExecuteCommandLists(&command_list, 1);
    device::Flush();
}

static void editor::Shutdown()
{
    for (u32 i = 0; i < (u32)arrlen(window_list); ++i)
    {
        // TODO(Dustin): Serialize window settings?
        window_list[i]->OnClose();
        SysFree(window_list[i]);
    }
    arrfree(window_list);
}

static void 
editor::DrawTerrainViewport()
{
    ImGui::Begin("Terrain Viewport");
    ImGui::End();
}

static void 
editor::DrawTerrainSettings()
{
    ImGui::Begin("Terrain Settings");
    ImGui::End();
}

static void
editor::WindowRegisterCallbacks(WND_ID wnd)
{
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_ButtonPress, ButtonPressCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_ButtonRelease, ButtonReleaseCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_KeyPressed, KeyPressCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_KeyRelease, KeyReleaseCallback);
    HostWndAttachListener(wnd,
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_Close, CloseCallback);
}

static void 
editor::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("TabTest", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);
    
    static ImGuiDockNodeFlags dockspace_flags = 
        ImGuiDockNodeFlags_PassthruCentralNode|
        ImGuiDockNodeFlags_NoTabBar|
        ImGuiDockNodeFlags_HiddenTabBar|
        ImGuiDockNodeFlags_NoCloseButton;
    
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    ImGuiID dockspace_id = ImGui::GetID("RootDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    static bool first_time = true;
    if (first_time)
    {
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGui::DockBuilderDockWindow("CoreWindow", dockspace_id);
        
        ImGuiDockNode *node = ImGui::DockBuilderGetNode(dockspace_id);
        node->LocalFlags = ImGuiDockNodeFlags_NoTabBar|ImGuiDockNodeFlags_NoCloseButton|ImGuiDockNodeFlags_NoResizeY;
        
        ImGui::DockBuilderFinish(dockspace_id);
        
        //-----------------------------------------------------------------------------------//
        // Init test windows
        
        MainViewportWindow *main_window = 0;
        TerrainGenWindow   *tg_window = 0;
        
        void *mem = SysAlloc(sizeof(MainViewportWindow));
        main_window = new (mem)MainViewportWindow();
        main_window->OnInit("Main Window");
        
        mem = SysAlloc(sizeof(TerrainGenWindow));
        tg_window = new (mem)TerrainGenWindow();
        tg_window->OnInit("Terrain Gen");
        
        arrput(window_list, main_window);
        arrput(window_list, tg_window);
        
        first_time = false;
    }
    
    
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows, 
            // which we can't undo at the moment without finer window depth/z control.
            //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
            if (ImGui::MenuItem("New", "Ctrl+N")) {}
            //NewScene();
            
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {}
            //OpenScene();
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
            //SaveSceneAs();
            
            if (ImGui::MenuItem("Exit")) 
                PlatformCloseApplication();
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Options"))
        {
            if (!g_show_imgui_metrics)
            {
                if (ImGui::MenuItem("Show Metrics", "Ctrl+M")) 
                {
                    void *mem = SysAlloc(sizeof(MetricsWindow));
                    MetricsWindow *window = new (mem)MetricsWindow();
                    window->OnInit(METRICS_WND_NAME);
                    arrput(window_list, window);
                    g_show_imgui_metrics = true;
                }
            }
            else
            {
                if (ImGui::MenuItem("Hide Metrics", "Ctrl+M")) 
                {
                    for (u32 i = 0; i < (u32)arrlen(window_list); ++i)
                    {
                        if (strcmp(StrGetString(&window_list[i]->_name), METRICS_WND_NAME) == 0)
                        {
                            WindowInterface *wnd = window_list[i];
                            arrdel(window_list, i);
                            SysFree(wnd);
                            selected_window = fast_max(0, selected_window-1);
                            break;
                        }
                    }
                    g_show_imgui_metrics = false;
                }
            }
            
            if (!g_show_imgui_demo)
            {
                if (ImGui::MenuItem("Show Demo Window", "Ctrl+M")) 
                {
                    void *mem = SysAlloc(sizeof(DemoWindow));
                    DemoWindow *window = new (mem)DemoWindow();
                    window->OnInit(DEMO_WND_NAME);
                    arrput(window_list, window);
                    g_show_imgui_demo = true;
                }
            }
            else
            {
                if (ImGui::MenuItem("Hide Demo Window", "Ctrl+M")) 
                {
                    for (u32 i = 0; i < (u32)arrlen(window_list); ++i)
                    {
                        if (strcmp(StrGetString(&window_list[i]->_name), DEMO_WND_NAME) == 0)
                        {
                            WindowInterface *wnd = window_list[i];
                            arrdel(window_list, i);
                            SysFree(wnd);
                            selected_window = fast_max(0, selected_window-1);
                            break;
                        }
                    }
                    g_show_imgui_demo = false;
                }
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    bool open = true;
    
    ImGui::Begin("CoreWindow", &open, ImGuiWindowFlags_NoDecoration);
    
    ImGui::BeginChild("One", ImVec2(0, 40), true, ImGuiWindowFlags_NoDecoration);
    {
        // TODO(Dustin): 
        // --- Close tabs
        // --- What happens when there are too many tabs open?
        // --- Round buttons
        
        for (int n = 0; n < arrlen(window_list); n++)
        {
            ImGui::PushID(n);
            ImGui::SameLine();
            
            const ImVec2 max_button_sz = ImVec2(256, 20);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            
            char *title = StrGetString(&window_list[n]->_name);
            
            ImVec2 text_size = ImGui::CalcTextSize(title, NULL, false, 0.0f);
            text_size.x = fast_minf(max_button_sz.x, text_size.x);
            text_size.y = max_button_sz.y;
            
            ImVec2 button_padding = ImVec2(10, 2);
            ImVec2 button_size    = ImVec2(text_size.x + button_padding.x, text_size.y + button_padding.y);
            
            if (ImGui::Button(title, button_size))
            {
                selected_window = n;
            }
            
            // TODO(Dustin): The correct way to do this is to draw an invisible button 
            // when we hover over a tab, and draw the small button on the top right of
            // the invis button. Want to only show the close button if we hover over the tab.
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
            {
                WindowInterface *wnd = window_list[n];
                arrdel(window_list, n);
                wnd->OnClose();
                SysFree(wnd);
                
                if (arrlen(window_list) == 0) selected_window = -1;
                else selected_window = fast_max(0, selected_window-1);
                
                --n;
                ImGui::PopID();
                continue;
            }
            
            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));
                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                // the filename and a small preview of the image, etc.)
                ImGui::Text("Swap %s", title);
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(int));
                    int payload_n = *(const int*)payload->Data;
                    WindowInterface* tmp = window_list[n];
                    window_list[n] = window_list[payload_n];
                    window_list[payload_n] = tmp;
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();
            
            if (n == selected_window)
            {
                ImVec2 marker_min = ImVec2(pos.x, pos.y);
                ImVec2 marker_max = ImVec2(pos.x + button_size.x, pos.y + button_size.y);
                
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRect(marker_min,  // upper left
                                   marker_max,  // lower right
                                   ImGui::GetColorU32(ImVec4(1.0f,0,0,1.0f)), 
                                   3.0f,        // rounding
                                   0,           // ImDrawFlags 
                                   1.2f);       // thickness
            }
        }
    }
    ImGui::EndChild();
    
    ImGui::BeginChild("Two", ImVec2(0,0), false, ImGuiWindowFlags_NoDecoration);
    {
        if (selected_window >= 0) window_list[selected_window]->OnRender();
    }
    ImGui::EndChild();
    
    ImGui::End();
    
    //if (g_show_imgui_metrics)
    //ImGui::ShowMetricsWindow(&g_show_imgui_metrics);
    
    ImGui::End(); // TabTest
    
    // End imgui frame
    ImGui::Render();
}
