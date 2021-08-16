
    struct MainViewportWindow : public WindowInterface
{
    static constexpr char *c_dockspace_name       = "Dockspace";
    static constexpr char *c_scene_name           = "Scene";
    static constexpr char *c_content_browser_name = "Content Browser";
    static constexpr char *c_viewport_name        = "Main Viewport";
    
    Str                _dockspace_name;
    Str                _scene_wnd_name;
    Str                _content_browser_wnd_name;
    Str                _viewport_wnd_name;
    
    bool               _dirty;
    // TODO(Dustin): Booleans to determine which subwindows are open
    
    v2                 _viewport_bounds[2];
    v2                 _viewport_size = V2_ZERO;
    bool               _viewport_focused = false;
    bool               _viewport_hovered = false;
    RenderTarget       _rt_viewport;
    TEXTURE_ID         _color_texture = INVALID_TEXTURE_ID;
    TEXTURE_ID         _depth_texture = INVALID_TEXTURE_ID;
    ViewportCamera     _camera;
    ImGuiID            _viewport_id;
    
    FILE_ID           _directory_fid;
    
    virtual void OnInit(const char *name);
    virtual void OnClose();
    virtual void OnRender();
    /* Keyboard/Mouse handling for windows with a viewport */
    virtual bool KeyPressCallback(input_layer::Event event, void *args);
    virtual bool KeyReleaseCallback(input_layer::Event event, void *args);
    virtual bool ButtonPressCallback(input_layer::Event event, void *args);
    virtual bool ButtonReleaseCallback(input_layer::Event event, void *args);
    
    void DrawContentBrowser();
    void DrawViewport();
};

void MainViewportWindow::OnInit(const char *name)
{
    //-------------------------------------------------------------------------------------------//
    // Setup the names for each window/dockspace
    
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
        u64 len = name_len + 3 + 2*strlen(c_scene_name);
        _scene_wnd_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_scene_wnd_name);
        ImFormatString(ptr, len, "%s###%s%s", c_scene_name, name, c_scene_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_content_browser_name);
        _content_browser_wnd_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_content_browser_wnd_name);
        ImFormatString(ptr, len, "%s###%s%s", c_content_browser_name, name, c_content_browser_name);
    }
    
    {
        u64 offset = 0;
        u64 len = name_len + 3 + 2*strlen(c_viewport_name);
        _viewport_wnd_name = StrInit(len, NULL);
        char *ptr = StrGetString(&_viewport_wnd_name);
        ImFormatString(ptr, len, "%s###%s%s", c_viewport_name, name, c_viewport_name);
    }
    
    //-------------------------------------------------------------------------------------------//
    // Setup the dockspace
    
    //-------------------------------------------------------------------------------------------//
    // Initialize the viewport
    
    // TODO(Dustin): 
    // The core rendering code should be place in the renderer - not the UI
    // Instead, allow the UI to ask for the last rendered image.
    
#if 0
    
    _rt_viewport.Init();
    _viewport_bounds[0] = V2_ZERO;
    _viewport_bounds[1] = V2_ZERO;
    
    v3 eye_pos = { 0, 0, -10 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    _camera.Init(eye_pos, up_dir);
    
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    _viewport_size = { viewport_panel_size.x, viewport_panel_size.y };
    
    // Resize the depth texture.
    D3D12_RESOURCE_DESC depthTextureDesc = d3d::GetTex2DDesc(DXGI_FORMAT_D32_FLOAT, 
                                                             (u32)_viewport_size.x, 
                                                             (u32)_viewport_size.y);
    // Must be set on textures that will be used as a depth-stencil buffer.
    depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    // Specify optimized clear values for the depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil      = { 1.0F, 0 };
    
    _depth_texture = texture::Create(&depthTextureDesc, &optimizedClearValue);
    
    DXGI_FORMAT back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(back_buffer_format, 
                                                           (u32)_viewport_size.x, 
                                                           (u32)_viewport_size.y);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    color_tex_desc.MipLevels = 1;
    
    D3D12_CLEAR_VALUE color_clear_value;
    color_clear_value.Format   = color_tex_desc.Format;
    color_clear_value.Color[0] = 0.4f;
    color_clear_value.Color[1] = 0.6f;
    color_clear_value.Color[2] = 0.9f;
    color_clear_value.Color[3] = 1.0f;
    
    _color_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
    _rt_viewport.Resize((u32)_viewport_size.x, (u32)_viewport_size.y);
    _rt_viewport.AttachTexture(AttachmentPoint::Color0, _color_texture);
    _rt_viewport.AttachTexture(AttachmentPoint::DepthStencil, _depth_texture);
    
    g_tile_settings.tile_x           = 1;
    g_tile_settings.tile_y           = 1;
    g_tile_settings.vertex_y         = 16;
    g_tile_settings.vertex_x         = 16;
    g_tile_settings.scale            = 1.0f;
    g_tile_settings.texture_tiling   = 1.0f;
    g_tile_settings.meshing_strategy = TerrainMeshType::TriangleStrip;
    
    g_terrain_sim.scale      = 0.002f;
    g_terrain_sim.seed       = 534864359;
    g_terrain_sim.octaves    = 8;
    g_terrain_sim.lacunarity = 2.0f;
    g_terrain_sim.decay      = 0.5f;
    g_terrain_sim.threshold  = -1.0f;
    
    //terrain::ExecuteFunction(terrain::Function_Perlin, &g_terrain_sim, command_list, g_heightmap);
    //g_hm_downsampler.Downsample(command_list, g_heightmap, g_downsampled_heightmap);
    
#endif
    
    //-------------------------------------------------------------------------------------------//
    // Initialize the Content Browser
    
    _directory_fid = PlatformGetMountFile("project");
    
    // If the current directory isn't the content directory, move into it
    PlatformFile* file = PlatformGetFile(_directory_fid);
    
    if (StrCmp(&file->relative_name, "Content") != 0)
    {
        bool found = false;
        for (u32 i = 0; i < (u32)arrlen(file->child_fids); ++i)
        {
            PlatformFile* child_file = PlatformGetFile(file->child_fids[i]);
            if (StrCmp(&child_file->relative_name, "Content") == 0)
            {
                _directory_fid = child_file->fid;
                found = true;
                break;
            }
        }
        Assert(found);
    }
}

void MainViewportWindow::OnClose()
{
    StrFree(&_name);
    StrFree(&_dockspace_name);
    StrFree(&_scene_wnd_name);
    StrFree(&_content_browser_wnd_name);
    StrFree(&_viewport_wnd_name);
}

void MainViewportWindow::OnRender()
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
        
        // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
        //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
        //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
        auto dock_id_left     = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
        auto dock_id_down     = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
        
        // we now dock our windows into the docking node we made above
        //ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
        ImGui::DockBuilderDockWindow(StrGetString(&_scene_wnd_name), dock_id_left);
        ImGui::DockBuilderDockWindow(StrGetString(&_content_browser_wnd_name), dock_id_down);
        ImGui::DockBuilderDockWindow(StrGetString(&_viewport_wnd_name), dockspace_id);
        
        ImGui::DockBuilderFinish(dockspace_id);
        
        first_time = false;
    }
    
    ImGui::Begin(StrGetString(&_scene_wnd_name));
    ImGui::Text("This is the scene");
    ImGui::End();
    
    DrawContentBrowser();
    DrawViewport();
}

void
MainViewportWindow::DrawViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    bool renderable = ImGui::Begin(StrGetString(&_viewport_wnd_name));
    
#if 0
    ImGuiIO& io = ImGui::GetIO();
    
    auto viewport_min_region = ImGui::GetWindowContentRegionMin();
    auto viewport_max_region = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    g_viewport_bounds[0] = { viewport_min_region.x + viewportOffset.x, viewport_min_region.y + viewportOffset.y };
    g_viewport_bounds[1] = { viewport_max_region.x + viewportOffset.x, viewport_max_region.y + viewportOffset.y };
    
    g_viewport_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
    g_viewport_hovered = ImGui::IsWindowHovered();
    
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    g_viewport_size = { viewport_panel_size.x, viewport_panel_size.y };
    
#if 0
    static bool first_time = true;
    if (first_time)
    {
        ImGuiViewport *viewport = ImGui::GetWindowViewport();
        ImGuiID viewport_id = viewport->ID;
        // The viewport has changed, so need to register callbacks...
        PlatformViewportData *viewport_data = (PlatformViewportData*)viewport->PlatformUserData;
        if (viewport_data)
        {
            first_time = false;
            WindowRegisterCallbacks(viewport_data->Wnd);
            g_main_viewport_id = viewport_id;
        }
        else
        {
            renderable = false;
        }
    }
#endif
    
    if (renderable)
    {
        // Update the camera (if it needs to)
        g_main_camera.OnUpdate(io.DeltaTime, { io.MouseDelta.x, io.MouseDelta.y });
        g_main_camera.OnMouseScroll(io.MouseWheel);
        
        // Setup render target
        g_rt_viewport.Resize((u32)g_viewport_size.x, (u32)g_viewport_size.y);
        
        // Setup view projection matrix
        r32 aspect_ratio;
        if (g_viewport_size.x >= g_viewport_size.y)
        {
            aspect_ratio = (r32)g_viewport_size.x / (r32)g_viewport_size.y;
        }
        else 
        {
            aspect_ratio = (r32)g_viewport_size.y / (r32)g_viewport_size.x;
        }
        
        m4 projection_matrix = m4_perspective(g_main_camera._zoom, aspect_ratio, 0.1f, 100.0f);
        m4 view_matrix = g_main_camera.LookAt();
        m4 view_proj = m4_mul(projection_matrix, view_matrix);
        // Draw!
        
        // TODO(Dustin): Move this into a "DrawScene"-like function
        
        CommandList *command_list = RendererGetActiveCommandList();
        
        // Update terrain Mesh, conditions:
        // ---- LMB is not pressed
        // ---- Terrain has been marked as dirty
        // NOTE(Dustin): Should this go on the COPY command queue instead?
        if (((g_mouse_button_mask & Button_0) == 0) && g_terrain_dirty)
        {
            // @FIXME
            // this isn't a great thing to do, but we cannot release a Resource
            // when it is potentially in use on a CommandList
            device::Flush();
            
            g_terrain.Generate(command_list, &g_tile_settings, NULL);
            g_terrain_dirty = false;
        }
        
        if (((g_mouse_button_mask & Button_0) == 0) && g_terrain_gen_dirty)
        {
            terrain::ExecuteFunction(terrain::Function_Perlin, &g_terrain_sim, command_list, g_heightmap);
            g_hm_downsampler.Downsample(command_list, g_heightmap, g_downsampled_heightmap);
            g_terrain_gen_dirty = false;
        }
        
        D3D12_VIEWPORT viewport = g_rt_viewport.GetViewport();
        command_list->SetViewport(viewport);
        
        D3D12_RECT scissor_rect = {};
        scissor_rect.left   = 0;
        scissor_rect.top    = 0;
        scissor_rect.right  = (LONG)viewport.Width;
        scissor_rect.bottom = (LONG)viewport.Height;
        command_list->SetScissorRect(scissor_rect);
        
        r32 clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        command_list->ClearTexture(g_rt_viewport.GetTexture(AttachmentPoint::Color0), clear_color);
        command_list->ClearDepthStencilTexture(g_rt_viewport.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
        
        command_list->SetRenderTarget(&g_rt_viewport);
        
        // Draw the terrain
        //g_terrain.Render(command_list, view_proj, g_heightmap);
        
        TEXTURE_ID texture = g_rt_viewport.GetTexture(AttachmentPoint::Color0);
        ImGui::Image((ImTextureID)((uptr)texture.val), 
                     ImVec2{ g_viewport_size.x, g_viewport_size.y }, 
                     ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }
    
#endif
    
    ImGui::End(); // Viewport end
    ImGui::PopStyleVar();
}

void
MainViewportWindow::DrawContentBrowser()
{
    ImGui::Begin(StrGetString(&_content_browser_wnd_name));
    
    // Styling
    ImGuiStyle& style = ImGui::GetStyle();
    const ImVec4 window_bg  = style.Colors[ImGuiCol_WindowBg];
    const ImVec4 hovered_bg = style.Colors[ImGuiCol_ButtonHovered];
    
    // Current Directory
    PlatformFile* file = PlatformGetFile(_directory_fid);
    
    //---------------------------------------------------------------------------//
    // Panel Header
    
    static bool s_up_arrow_hovered = false;
    
    if (ImGui::ImageButton((ImTextureID)((uptr)g_icons[Icon_UpArrow].val), 
                           { 16, 16 }, 
                           {  0,  0 }, 
                           {  1,  1 }, 0, 
                           (s_up_arrow_hovered) ? hovered_bg : window_bg))
    {
        // TODO(Dustin): Hardcoded path...
        //if (PlatformIsValidFid(file->parent_fid))
        if (StrCmp(&file->relative_name, "Content") != 0)
        {
            _directory_fid = file->parent_fid;
            file = PlatformGetFile(_directory_fid);
        }
    }
    
    if (ImGui::IsItemHovered())
    {
        s_up_arrow_hovered = true;
    }
    else
        s_up_arrow_hovered = false;
    
    ImGui::SameLine();
    
    // TODO(Dustin): Implement undo & redo actions
    ImVec4 arrow_bg = window_bg;
    arrow_bg.w = 0.2f;
    
    ImGui::ImageButton((ImTextureID)((uptr)g_icons[Icon_LeftArrow].val), 
                       { 16, 16 }, 
                       {  0,  0 }, 
                       {  1,  1 }, 0, 
                       window_bg,
                       { 1, 1, 1, 0.2f });
    ImGui::SameLine();
    ImGui::ImageButton((ImTextureID)((uptr)g_icons[Icon_RightArrow].val), {16, 16},
                       { 0, 0 }, 
                       { 1, 1 }, 0, 
                       window_bg,
                       { 1, 1, 1, 0.2f });
    
    ImGui::Separator();
    
    //---------------------------------------------------------------------------//
    // Panel Content
    
    static i32 hovered_idx = -1;
    bool is_item_hovered = false;
    
    const r32 thumbnail_sz = 64.0f;
    const r32 padding = 16.0f;
    const r32 cell_sz = thumbnail_sz + padding;
    const r32 content_region_x = ImGui::GetContentRegionAvail().x;
    i32 column_count = fast_max(1, (i32)(content_region_x / cell_sz));
    
    ImGui::Columns(column_count, 0, false);
    
    for (u32 i = 0; i < (u32)arrlen(file->child_fids); ++i)
    {
        PlatformFile* child_file = PlatformGetFile(file->child_fids[i]);
        
        // Extract the filename from the relative path...
        const char *filename = 0;
        {
            const char *relative_name = StrGetString(&child_file->relative_name);
            u64 len = StrLen(&child_file->relative_name);
            
            u64 i = 0;
            for (i = len; i > 0; --i)
            {
                if (relative_name[i-1] == '/') 
                    break;
            }
            
            filename = relative_name + i;
        }
        
        if (child_file->type == FileType::Directory)
        {
            ImGui::ImageButton((ImTextureID)((uptr)g_icons[Icon_Directory].val), 
                               {thumbnail_sz, thumbnail_sz}, 
                               {0, 0}, {1,1}, 0, 
                               (i == hovered_idx) ? hovered_bg : window_bg);
            
            if (ImGui::IsItemHovered())
            {
                hovered_idx = i;
                is_item_hovered = true;
                
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    _directory_fid = file->child_fids[i];
                }
            }
        }
        else
        {
            ImGui::ImageButton((ImTextureID)((uptr)g_icons[Icon_File].val), {thumbnail_sz, thumbnail_sz}, 
                               {0, 0}, {1,1}, 0,
                               (i == hovered_idx) ? hovered_bg : window_bg);
            
            if (ImGui::IsItemHovered())
            {
                hovered_idx = i;
                is_item_hovered = true;
            }
        }
        
        if (i == hovered_idx)
        {
            // Draw hovered background
            
            r32 spacing_y = style.ItemSpacing.y;
            
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            const float wrap_pos_x = window->DC.TextWrapPos;
            const bool wrap_enabled = (wrap_pos_x >= 0.0f);
            const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x);
            //const float wrap_width = thumbnail_sz;
            
            ImVec2 text_size = ImGui::CalcTextSize(filename, NULL, false, thumbnail_sz);
            text_size.x = thumbnail_sz;
            
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 marker_min = ImVec2(pos.x + wrap_width, pos.y - spacing_y);
            ImVec2 marker_max = ImVec2(pos.x + text_size.x, 
                                       pos.y + text_size.y + (padding/2));
            
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(marker_min, marker_max, 
                                     ImGui::ColorConvertFloat4ToU32(hovered_bg));
        }
        
        ImGui::TextWrapped(filename);
        
        ImGui::NextColumn();
    }
    
    // Nothing is hovered, so do not 
    if (!is_item_hovered)
        hovered_idx = -1;
    
    ImGui::Columns(1);
    ImGui::End();
}

bool MainViewportWindow::KeyPressCallback(input_layer::Event event, void *args)
{
    return false;
}

bool MainViewportWindow::KeyReleaseCallback(input_layer::Event event, void *args)
{
    return false;
}

bool MainViewportWindow::ButtonPressCallback(input_layer::Event event, void *args)
{
    return false;
}

bool MainViewportWindow::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    return false;
}
