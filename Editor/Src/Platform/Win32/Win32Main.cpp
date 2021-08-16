
static const char      *g_editor_title = "Sapling Editor";
static const u32        g_window_width   = 1920;
static const u32        g_window_height  = 1080;
static bool             g_is_running = false; 

static Win32ThreadPool *g_thread_pool    = 0;
static const u64        g_internal_mem_sz = _MB(512);
static void            *g_internal_mem    = 0;
static bool             g_needs_resize = false;
static WND_ID           g_root_wnd = INVALID_WND_ID;

// Startup information
struct MapleProject
{
    Str name;
    Str filepath;
    // TODO(Dustin): Icon
};

static const char   *g_engine_startup_file = "startup.toml";
static Str           g_engine_content_dir;
static MapleProject *g_known_projects;
static u32           g_active_project;

void KeyPressCallback(MapleKey key)
{
    if (key == Key_Escape)
        g_is_running = false;
}

static bool 
WindowResizeCallback(input_layer::Event event, void *args) 
{
    g_needs_resize = true;
    return false;
}

void PlatformCloseApplication()
{
    g_is_running = false;
}

static void 
LoadProjectFile(MapleProject *project)
{
    const char *filename = "/maple.project";
    Str project_path = StrAdd(&project->filepath, filename, strlen(filename));
    
    Toml toml;
    TomlResult result = TomlLoad(&toml, StrGetString(&project_path));
    Assert(result == TomlResult_Success);
    
    project->name = StrInit(strlen(toml.title), toml.title);
    
    TomlFree(&toml);
    StrFree(&project_path);
}

static void
LoadStartupFile(const char *startup)
{
    Toml toml;
    TomlResult result = TomlLoad(&toml, startup);
    Assert(result == TomlResult_Success);
    
    TomlObject obj = TomlGetObject(&toml, "EngineStartup");
    
    g_engine_content_dir = StrInit(TomlGetStringLen(&obj, "engine_assets"), 
                                   TomlGetString(&obj, "engine_assets"));
    
    TomlData* proj_array = TomlGetArray(&obj, "known_projects");
    int len = TomlGetArrayLen(proj_array);
    for (int i = 0; i < len; ++i)
    {
        MapleProject project = {};
        project.filepath = StrInit(TomlGetStringLenArrayElem(proj_array, i), 
                                   TomlGetStringArrayElem(proj_array, i));
        
        LoadProjectFile(&project);
        
        arrput(g_known_projects, project);
    }
    
    g_active_project = TomlGetInt(&obj, "last_active_project");
    
    TomlFree(&toml);
}

static void 
Win32ReadFileToBuffer_Wrapper(const char* file_path, u8** buffer, int* size)
{
    PlatformReadFileToBuffer(file_path, buffer, (u32*)size);
}


INT WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    PlatformLoggerInit();
    GlobalTimerSetup();
    PlatformGetInvalidGuid(); // init the global invalid guid
    
    g_internal_mem = PlatformVirtualAlloc(g_internal_mem_sz);
    SysMemoryInit(g_internal_mem, g_internal_mem_sz);
    
    {
        TomlCallbacks callbacks = {};
        callbacks.Alloc    = SysMemoryAlloc;
        callbacks.Free     = SysMemoryRelease;
        callbacks.LoadFile = Win32ReadFileToBuffer_Wrapper;
        callbacks.FreeFile = SysMemoryRelease;
        TomlSetCallbacks(&callbacks);
    }
    
    LoadStartupFile(g_engine_startup_file);
    
    // initialize file manager
    file_manager::Init();
    file_manager::MountFile("engine",  StrGetString(&g_engine_content_dir));
    file_manager::MountFile("project", StrGetString(&g_known_projects[g_active_project].filepath));
    
    // Create the Root Window
    g_root_wnd = HostWndInit(g_window_width, g_window_height, StrGetString(&g_known_projects[g_active_project].name));
    // the root window should forward all events to ImGui proc handler before 
    // dispatching events to other systems
    HostWndAttachListener(g_root_wnd, input_layer::Layer_ImGui, input_layer::Event_AllEvents, Win32ImGuiWndProcHandler);
    HostWndAttachListener(g_root_wnd, input_layer::Layer_Game, input_layer::Event_Size, WindowResizeCallback);
    
    // Initialize the renderer
    
    RendererInitInfo renderer_info = {};
    renderer_info.wnd = HostWndGetHandle(g_root_wnd);
    HostWndGetDims(g_root_wnd, &renderer_info.wnd_width, &renderer_info.wnd_height);
    
    RendererInit(&renderer_info);
    terrain::LoadComputeFunctions();
    
    // 3. Initialize Editor (ImGui)
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
    
    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    
    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    
    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    
    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    
    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    
    Win32ImGuiInit(g_root_wnd);
    d3d_imgui::Initialize(g_swapchain.GetRenderTarget());
    editor::Initialize();
    
    
    // 5. Execution Loop
    HostWndSetActive(g_root_wnd);
    g_is_running = true;
    while (g_is_running)
    {
        // 5.1 Should we reload any modules?
        
        
        
        // 5.2 Process any pending messages
        
        if (!HostWndMsgLoop(g_root_wnd))
        {
            g_is_running = false;
            continue;
        }
        
        // Resize
        
        if (g_needs_resize)
        {
            g_needs_resize = false;
            
            u32 width, height;
            HostWndGetDims(g_root_wnd, &width, &height);
            RendererResize(width, height);
        }
        
        // 5.3 Execute Game Logic
        
        
        
        // 5.4 Render Game
        
        //CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        // NOTE(Dustin): The cause of quite a few objects being grabbed!
        //CommandList *command_list = command_queue->GetCommandList();
        
        //RendererClearRootSwapchain(command_list);
        
        RendererBeginFrame();
        
        // 5.5 Render Editor GUI
        
        // Begin imgui frame
        Win32ImGuiNewFrame();
        ImGui::NewFrame();
        editor::Render();
        
        // Draw into the current swapchain image
        d3d_imgui::Render(ImGui::GetDrawData(), RendererGetActiveCommandList(), g_swapchain.GetRenderTarget());
        RendererEndFrame();
        
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(NULL, NULL);
        }
        
        // Present
        
        //RendererPresentFrame();
        
        // 5.6 Meet Framerate (?)
        
        
        
    }
    
    // Release the engine
    
    // Flush any pending commands before resizing resources.
    device::Flush();
    editor::Shutdown();
    d3d_imgui::Free();
    terrain::ReleaseComputeFunctions();
    RendererFree();
    HostWndFree(g_root_wnd);
    
    SysMemoryFree();
    PlatformVirtualFree(g_internal_mem);
    PlatformLoggerFree();
    
    return (0);
}
