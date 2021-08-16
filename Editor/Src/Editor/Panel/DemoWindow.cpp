
struct DemoWindow : public WindowInterface
{
    static constexpr char *c_dockspace_name = "Dockspace";
    static constexpr char *c_demo_name      = "Dear ImGui Demo";
    
    Str _dockspace_name;
    
    virtual void OnInit(const char *name);
    virtual void OnClose();
    virtual void OnRender();
};

void 
DemoWindow::OnInit(const char *name)
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
}

void 
DemoWindow::OnClose()
{
    StrFree(&_dockspace_name);
    StrFree(&_name);
}

void 
DemoWindow::OnRender()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    ImGuiID dockspace_id = ImGui::GetID(StrGetString(&_dockspace_name));
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    static bool first_time = true;
    if (first_time)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        // Sets central node?
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        
        ImGui::DockBuilderDockWindow(c_demo_name, dockspace_id);
        
        first_time = false;
    }
    
    ImGui::ShowDemoWindow(&g_show_imgui_demo);
}
