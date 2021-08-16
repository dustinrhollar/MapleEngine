
namespace ed = ax::NodeEditor;
struct TerrainGenWindow : public WindowInterface
{
    static constexpr char *c_dockspace_name  = "Dockspace";
    static constexpr char *c_settings_name   = "Settings";
    static constexpr char *c_console_name    = "Console";
    static constexpr char *c_viewport_name   = "Viewport";
    static constexpr char *c_graph_name      = "Graph";
    
    Str                _dockspace_name;
    Str                _settings_name;
    Str                _console_name;
    Str                _viewport_name;
    Str                _graph_name;
    
    ImGuiDockNodeFlags _dockspace_flags;
    
    ed::EditorContext     *_editor_context = 0;
    
    ViewportCamera     _camera;
    ImGuiID            _viewport_id;
    
    
    bool                   _dirty;
    // TODO(Dustin): Booleans to determine which subwindows are open
    
    virtual void OnInit(const char *name);
    virtual void OnClose();
    virtual void OnRender();
    /* Keyboard/Mouse handling for windows with a viewport */
    virtual bool KeyPressCallback(input_layer::Event event, void *args);
    virtual bool KeyReleaseCallback(input_layer::Event event, void *args);
    virtual bool ButtonPressCallback(input_layer::Event event, void *args);
    virtual bool ButtonReleaseCallback(input_layer::Event event, void *args);
    
    void DrawTerrainGraph();
    
};

void 
TerrainGenWindow::OnInit(const char *name)
{
    u64 name_len = strlen(name);
    _name = StrInit(name_len, name);
    
    {
        u64 offset = 0;
        u64 len = name_len + strlen(c_dockspace_name);
        _dockspace_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_dockspace_name);
        ImFormatString(ptr, len, "%s%s", name, c_dockspace_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_settings_name);
        _settings_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_settings_name);
        ImFormatString(ptr, len, "%s###%s%s", c_settings_name, name, c_settings_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_console_name);
        _console_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_console_name);
        ImFormatString(ptr, len, "%s###%s%s", c_console_name, name, c_console_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_viewport_name);
        _viewport_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_viewport_name);
        ImFormatString(ptr, len, "%s###%s%s", c_viewport_name, name, c_viewport_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_graph_name);
        _graph_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_graph_name);
        ImFormatString(ptr, len, "%s###%s%s", c_graph_name, name, c_graph_name);
    }
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    _dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (_dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    v3 eye_pos = { 0, 0, -10 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    _camera.Init(eye_pos, up_dir);
    
    _editor_context = ed::CreateEditor();
}

void 
TerrainGenWindow::OnClose()
{
}

void 
TerrainGenWindow::OnRender()
{
    ImGuiID dockspace_id = ImGui::GetID(StrGetString(&_dockspace_name));
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), _dockspace_flags);
    
    static bool first_time = true;
    if (first_time)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, _dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        
        auto dock_id_viewport = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
        auto dock_id_settings = ImGui::DockBuilderSplitNode(dock_id_viewport, ImGuiDir_Down, 0.60f, nullptr, &dock_id_viewport);
        auto dock_id_console  = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
        
        ImGui::DockBuilderDockWindow(StrGetString(&_viewport_name), dock_id_viewport);
        ImGui::DockBuilderDockWindow(StrGetString(&_settings_name), dock_id_settings);
        ImGui::DockBuilderDockWindow(StrGetString(&_console_name),  dock_id_console);
        ImGui::DockBuilderDockWindow(StrGetString(&_graph_name),    dockspace_id);
        
        ImGui::DockBuilderFinish(dockspace_id);
        
        first_time = false;
    }
    
    ImGui::Begin(StrGetString(&_viewport_name));
    {
        ImGui::Text("Hello, viewport!");
        ImGui::Value("Viewport \"Viewport\" ID: ", ImGui::GetWindowViewport()->ID);
    }
    ImGui::End();
    
    ImGui::Begin(StrGetString(&_settings_name));
    {
        ImGui::Text("Hello, settings!");
        ImGui::Value("Viewport \"Settings\" ID: ", ImGui::GetWindowViewport()->ID);
    }
    ImGui::End();
    
    ImGui::Begin(StrGetString(&_console_name));
    {
        ImGui::Text("Hello, console!");
        ImGui::Value("Viewport \"Console\" ID: ", ImGui::GetWindowViewport()->ID);
    }
    ImGui::End();
    
    ImGui::Begin(StrGetString(&_graph_name));
    {
        DrawTerrainGraph();
    }
    ImGui::End();
}

void 
TerrainGenWindow::DrawTerrainGraph()
{
    ed::SetCurrentEditor(_editor_context);
    ed::Begin("My Editor", ImVec2(0.0, 0.0f));
    int uniqueId = 1;
    // Start drawing nodes.
    ed::BeginNode(uniqueId++);
    ImGui::Text("Node A");
    ed::BeginPin(uniqueId++, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    ImGui::SameLine();
    ed::BeginPin(uniqueId++, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    ed::EndNode();
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

bool 
TerrainGenWindow::KeyPressCallback(input_layer::Event event, void *args)
{
    return false;
}

bool 
TerrainGenWindow::KeyReleaseCallback(input_layer::Event event, void *args)
{
    return false;
}

bool 
TerrainGenWindow::ButtonPressCallback(input_layer::Event event, void *args)
{
    return false;
}

bool 
TerrainGenWindow::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    return false;
}
