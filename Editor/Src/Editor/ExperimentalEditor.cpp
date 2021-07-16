// Editor for experimental rendering features

namespace experimental
{
    struct Cube
    {
        enum RootParams { Param_MatrixCB, Param_Count };
        struct MatrixCB { m4 matrix; };
        
        struct Vertex
        {
            v3 pos, col;
            v2 tex;
        };
        
        m4                  _model;
        VertexBuffer        _vbuffer;
        IndexBuffer         _ibuffer;
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        
        void Init(v3 pos = {0,0,0});
        void Free();
        void Render(CommandList *command_list, m4 proj_view);
    };
    
    struct EnvMapCube
    {
        enum RootParams { Param_MatrixCB, Param_CameraCB, Param_Skybox, Param_Count };
        struct MatrixCB { m4 vp; m4 model; };
        struct CameraCB { v3 cam; };
        
        struct Vertex
        {
            v3 pos, norm;
            v2 tex;
        };
        
        m4                  _model;
        VertexBuffer        _vbuffer;
        IndexBuffer         _ibuffer;
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        
        void Init(v3 pos = {0,0,0});
        void Free();
        void Render(CommandList *command_list, m4 proj_view);
    };
    
    // Viewport Information
    
    static bool              g_viewport_focused;
    static bool              g_viewport_hovered;
    
    // Render Target
    static bool              g_render_msaa = true;
    static const DXGI_FORMAT g_back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    static DXGI_SAMPLE_DESC  g_msaa_sample_desc;
    static TEXTURE_ID        g_final_color_texture       = INVALID_TEXTURE_ID;
    static TEXTURE_ID        g_multisample_color_texture = INVALID_TEXTURE_ID;
    static TEXTURE_ID        g_depth_texture             = INVALID_TEXTURE_ID;
    static RenderTarget      g_render_target             = {};
    
    // Cubemap!
    
    static wchar_t *g_skybox_shaders[] = {
        L"shaders/Skybox_Vtx.cso",
        L"shaders/Skybox_Pxl.cso"
    };
    
    struct Skybox
    {
        void Init(const char *file);
        void Free();
        void Render(CommandList *command_list, m4 view_proj);
        
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        ShaderResourceView  _srv;
        TEXTURE_ID          _cubemap;
        TEXTURE_ID          _texture;
        VertexBuffer        _vbuffer;
        IndexBuffer         _ibuffer;
    };
    
    static Skybox           g_skybox; 
    
    // Scene Data
    
    static wchar_t *g_env_mapping_shaders[] = {
        L"shaders/EnvMapping_Vtx.cso",
        L"shaders/EnvMapping_Pxl.cso"
    };
    
    static wchar_t *g_cube_shaders[] = {
        L"shaders/Cube_Vtx.cso",
        L"shaders/Cube_Pxl.cso"
    };
    
    editor::ViewportCamera   g_camera;
    static Cube              g_cube;
    static EnvMapCube        g_env_map_cube;
    
    static void InitializeWindow(u32 width, u32 height);
    static void FreeWindow();
    static void DrawWindow();
    static void RenderScene(m4 proj, m4 view);
    
    static void DrawScene();
    static void DrawContentBrowser();
    
    // Callbacks
    static void WindowRegisterCallbacks(WND_ID wnd);
    static bool ButtonPressCallback(input_layer::Event event, void *args);
    static bool ButtonReleaseCallback(input_layer::Event event, void *args);
    static bool KeyPressCallback(input_layer::Event event, void *args);
    static bool KeyReleaseCallback(input_layer::Event event, void *args);
    static bool CloseCallback(input_layer::Event event, void *args);
    
};

static void 
CreateCube(CommandList *command_list,  VertexBuffer *vbuffer, IndexBuffer *ibuffer,
           r32 s = 1.0f, bool reverse_winding = false)
{
    struct Vertex
    {
        v3 pos;
        v3 norm;
        v2 tex;
    };
    
    // 8 edges of cube.
    v3 p[8] = { { s, s, -s }, { s, s, s },   { s, -s, s },   { s, -s, -s },
        { -s, s, s }, { -s, s, -s }, { -s, -s, -s }, { -s, -s, s } };
    // 6 face normals
    v3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    // 4 unique texture coordinates
    v2 t[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    
    // Indices for the vertex positions.
    uint16_t i[24] = {
        0, 1, 2, 3,  // +X
        4, 5, 6, 7,  // -X
        4, 1, 0, 5,  // +Y
        2, 7, 6, 3,  // -Y
        1, 4, 7, 2,  // +Z
        5, 0, 3, 6   // -Z
    };
    
    Vertex vertices[24];
    u16 indices[36];
    
    u16 vidx = 0;
    u16 iidx = 0;
    for (u16 f = 0; f < 6; ++f )  // For each face of the cube.
    {
        // Four vertices per face.
        vertices[vidx++] = { p[i[f * 4 + 0]], n[f], t[0] };
        vertices[vidx++] = { p[i[f * 4 + 1]], n[f], t[1] };
        vertices[vidx++] = { p[i[f * 4 + 2]], n[f], t[2] };
        vertices[vidx++] = { p[i[f * 4 + 3]], n[f], t[3] };
        
        // First triangle.
        indices[iidx++] =  f * 4 + 0;
        indices[iidx++] =  f * 4 + 1;
        indices[iidx++] =  f * 4 + 2;
        
        // Second triangle.
        indices[iidx++] =  f * 4 + 2;
        indices[iidx++] =  f * 4 + 3;
        indices[iidx++] =  f * 4 + 0;
    }
    
    if (reverse_winding)
    {
        for (u16 i = 0; i < _countof(indices); i += 3)
        {
            u16 _x = indices[i];
            u16 _y = indices[i+2];
            
            indices[i]   = _y;
            indices[i+2] = _x;
        }
        
        for (u32 i = 0; i < _countof(vertices); ++i)
        {
            vertices[i].tex.x = (1.0f - vertices[i].tex.x);
        }
    }
    
    command_list->CopyVertexBuffer(vbuffer, _countof(vertices), sizeof(Vertex), vertices);
    command_list->CopyIndexBuffer(ibuffer, _countof(indices), sizeof(u16), indices);
}

void 
experimental::Skybox::Init(const char *file)
{
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *copy_list = copy_queue->GetCommandList();
    
    CreateCube(copy_list, &_vbuffer, &_ibuffer, 5.0f, true);
    
    stbi_set_flip_vertically_on_load(true);  
    _texture = copy_list->LoadTextureFromFile(file);
    stbi_set_flip_vertically_on_load(false);
    assert(_texture != INVALID_TEXTURE_ID);
    
    D3D12_RESOURCE_DESC desc = texture::GetResourceDesc(_texture);
    desc.Width = desc.Height = 1024;
    desc.DepthOrArraySize = 6;
    desc.MipLevels = 0;
    _cubemap = texture::Create(&desc);
    
    // Convert 2D panoramic to a 3D cubemap
    copy_list->PanoToCubemap(_cubemap, _texture);
    copy_queue->ExecuteCommandLists(&copy_list, 1);
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format                          = desc.Format;
    srv_desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MipLevels           = (UINT)-1;  // Use all mips.
    _srv.Init(texture::GetResource(_cubemap), &srv_desc);
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
    };
    
    D3D12_DESCRIPTOR_RANGE1 range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    
    D3D12_ROOT_PARAMETER1 root_params[2];
    root_params[0] = d3d::root_param1::InitAsConstant(sizeof(m4)/4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_params[1] = d3d::root_param1::InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
    
    // Diffuse texture sampler
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, 
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(g_skybox_shaders[0]);
    shader_modules.pixel  = LoadShaderModule(g_skybox_shaders[1]);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &_root_signature;
    pso_desc.shader_modules      = shader_modules;
    pso_desc.blend               = GetBlendState(BlendState::Disabled);
    pso_desc.depth               = GetDepthStencilState(DepthStencilState::Disabled);
    
    pso_desc.rasterizer          = GetRasterizerState(RasterState::DefaultCw);
    pso_desc.rasterizer.DepthClipEnable = false;
    pso_desc.rasterizer.CullMode = D3D12_CULL_MODE_FRONT;
    
    pso_desc.input_layouts       = &input_desc[0];
    pso_desc.input_layouts_count = _countof(input_desc);
    pso_desc.topology            = GfxTopology::Triangle;
    pso_desc.sample_desc.count   = g_msaa_sample_desc.Count;
    pso_desc.sample_desc.quality = g_msaa_sample_desc.Quality;
    pso_desc.dsv_format          = GfxFormat::D32_Float;
    pso_desc.rtv_formats.NumRenderTargets = 1;
    pso_desc.rtv_formats.RTFormats[0]     = g_back_buffer_format;
    _pso.Init(&pso_desc);
}

void 
experimental::Skybox::Free()
{
    _vbuffer.Free();
    _srv.Free();
    _pso.Free();
    _root_signature.Free();
    texture::Free(_cubemap);
    // TODO(Dustin): Queue this up for destruction with the command list
    // that created it...
    texture::Free(_texture);
}

void 
experimental::Skybox::Render(CommandList *command_list, m4 view_proj)
{
    // Remove the transation part of the matrix
    //view_proj = m3_to_m4(m4_to_m3(view_proj));
    
    //m4 translate = m4_translate({0.5f, 0.5f, 0.f});
    //view_proj = m4_mul(translate, view_proj);
    
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    command_list->SetGraphics32BitConstants(0, &view_proj);
    command_list->SetShaderResourceView(1, 0, &_srv, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    command_list->SetVertexBuffer(0, &_vbuffer);
    command_list->SetIndexBuffer(&_ibuffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->DrawIndexedInstanced((UINT)_ibuffer._count);
}

static void 
experimental::InitializeWindow(u32 width, u32 height)
{
    // Check the best multisample quality level that can be used for the given back buffer format.
    g_msaa_sample_desc = device::GetMultisampleQualityLevels(g_back_buffer_format);
    
    D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(g_back_buffer_format, width, height, 1, 1, 
                                                           g_msaa_sample_desc.Count, g_msaa_sample_desc.Quality);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
    D3D12_CLEAR_VALUE color_clear_value;
    color_clear_value.Format   = color_tex_desc.Format;
    color_clear_value.Color[0] = 0.4f;
    color_clear_value.Color[1] = 0.6f;
    color_clear_value.Color[2] = 0.9f;
    color_clear_value.Color[3] = 1.0f;
    
    g_multisample_color_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
    color_tex_desc = d3d::GetTex2DDesc(g_back_buffer_format, width, height, 1, 1);
    g_final_color_texture = texture::Create(&color_tex_desc, NULL);
    
    // Resize the depth texture.
    D3D12_RESOURCE_DESC depthTextureDesc = d3d::GetTex2DDesc(DXGI_FORMAT_D32_FLOAT, width, height, 1, 1, 
                                                             g_msaa_sample_desc.Count, g_msaa_sample_desc.Quality);
    
    // Must be set on textures that will be used as a depth-stencil buffer.
    depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    // Specify optimized clear values for the depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil      = { 1.0F, 0 };
    g_depth_texture = texture::Create(&depthTextureDesc, &optimizedClearValue);
    
    g_render_target.Init();
    g_render_target.AttachTexture(AttachmentPoint::Color0, g_multisample_color_texture);
    g_render_target.AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
    
    // View matrix
    v3 eye_pos = { 0, 0, -10 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    g_camera.Init(eye_pos);
    
    // Cube to render
    g_cube.Init();
    g_env_map_cube.Init({5,0,0});
    
    //g_skybox.Init("textures/hdr/lilienstein_8k.hdr");
    //g_skybox.Init("textures/hdr/dikhololo_night_4k.hdr");
    //g_skybox.Init("textures/hdr/GCanyon_C_YumaPoint_Env.hdr");
    //g_skybox.Init("textures/hdr/Brooklyn_Bridge_Planks_Env.hdr");
    //g_skybox.Init("textures/hdr/Newport_Loft_Ref.hdr");
    g_skybox.Init("textures/hdr/christmas_photo_studio_01_4k.hdr");
}

static void 
experimental::RenderScene(m4 projection_matrix, m4 view_matrix)
{
    CommandList *command_list = RendererGetActiveCommandList();
    
    D3D12_VIEWPORT viewport = g_render_target.GetViewport();
    command_list->SetViewport(viewport);
    
    D3D12_RECT scissor_rect = {};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)viewport.Width;
    scissor_rect.bottom = (LONG)viewport.Height;
    command_list->SetScissorRect(scissor_rect);
    
    r32 clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearTexture(g_render_target.GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->ClearDepthStencilTexture(g_render_target.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    
    command_list->SetRenderTarget(&g_render_target);
    
    m4 view_proj = m4_mul(projection_matrix, m3_to_m4(m4_to_m3(view_matrix)));
    g_skybox.Render(command_list, view_proj);
    
    view_proj = m4_mul(projection_matrix, view_matrix);
    g_cube.Render(command_list, view_proj);
    g_env_map_cube.Render(command_list, view_proj);
    
    // Resolve the multisampled texture
    if (g_msaa_sample_desc.Count > 1)
    {
        Resource *src_texture = texture::GetResource(g_render_target.GetTexture(AttachmentPoint::Color0));
        Resource *dst_texture = texture::GetResource(g_final_color_texture);
        command_list->ResolveSubresource(dst_texture, src_texture);
    }
    else
    {
        Resource *src_texture = texture::GetResource(g_render_target.GetTexture(AttachmentPoint::Color0));
        Resource *dst_texture = texture::GetResource(g_final_color_texture);
        command_list->CopyResource(dst_texture, src_texture);
    }
}

static void 
experimental::DrawWindow()
{
    g_viewport_focused = ImGui::IsWindowFocused();
    g_viewport_hovered = ImGui::IsWindowHovered();
    
    static bool first_time = true;
    
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    
    if (first_time)
    {
        ImGuiViewport *viewport = ImGui::GetWindowViewport();
        PlatformViewportData *viewport_data = (PlatformViewportData*)viewport->PlatformUserData;
        if (viewport_data && viewport_panel_size.x > 0 && viewport_panel_size.y > 0)
        {
            first_time = false;
            WindowRegisterCallbacks(viewport_data->Wnd);
            InitializeWindow((u32)viewport_panel_size.x, (u32)viewport_panel_size.y);
        }
        else
        { // No window to draw into?
            return;
        }
    }
    
    // Update the camera (if it needs to)
    g_camera.OnUpdate(io.DeltaTime, { io.MouseDelta.x, io.MouseDelta.y });
    g_camera.OnMouseScroll(io.MouseWheel);
    
    g_render_target.Resize((u32)viewport_panel_size.x, (u32)viewport_panel_size.y);
    texture::Resize(g_final_color_texture, (u32)viewport_panel_size.x, (u32)viewport_panel_size.y);
    
    // Setup view projection matrix
    r32 aspect_ratio;
    if (viewport_panel_size.x >= viewport_panel_size.y)
    {
        aspect_ratio = (r32)viewport_panel_size.x / (r32)viewport_panel_size.y;
    }
    else 
    {
        aspect_ratio = (r32)viewport_panel_size.y / (r32)viewport_panel_size.x;
    }
    
    m4 projection_matrix = m4_perspective(g_camera._zoom, aspect_ratio, 0.1f, 100.0f);
    m4 view_matrix = g_camera.LookAt();
    //m4 view_proj = m4_mul(projection_matrix, view_matrix);
    
    RenderScene(projection_matrix, view_matrix);
    
    TEXTURE_ID texture;
    if (g_render_msaa)
        texture = g_render_target.GetTexture(AttachmentPoint::Color0);
    else
        texture = g_final_color_texture;
    
    texture = g_final_color_texture;
    ImGui::Image((ImTextureID)((uptr)texture.val), 
                 ImVec2{ viewport_panel_size.x, viewport_panel_size.y }, 
                 ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}

static void 
experimental::FreeWindow()
{
}

static void 
experimental::DrawScene()
{
    //ImGui::Checkbox("MSAA", &g_render_msaa);
}

static void 
experimental::DrawContentBrowser()
{
    ImGui::Text("Hello content browser");
}

static bool 
experimental::KeyPressCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_camera.OnKeyPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::KeyReleaseCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_camera.OnKeyRelease((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::ButtonPressCallback(input_layer::Event event, void *args)
{
    if (experimental::g_viewport_focused && experimental::g_viewport_hovered)
    {
        experimental::g_camera.OnMouseButtonPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    g_camera.OnMouseButtonRelease((MapleKey)event.key_press.key);
    return false;
}

static bool 
experimental::CloseCallback(input_layer::Event event, void *args)
{
    return false;
}

static void
experimental::WindowRegisterCallbacks(WND_ID wnd)
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

void 
experimental::Cube::Init(v3 pos)
{
    Vertex cube_vertices[8] = {
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // 0
        { { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } }, // 1
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }, // 2
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } }, // 3
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // 4
        { { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }, // 5
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // 6
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }  // 7
    };
    
    u16 cube_indicies[36] = { 0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7 };
    
    // Load vertex data!
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *copy_list = copy_queue->GetCommandList();
    
    copy_list->CopyVertexBuffer(&_vbuffer, _countof(cube_vertices), sizeof(Vertex), cube_vertices);
    copy_list->CopyIndexBuffer(&_ibuffer, _countof(cube_indicies), sizeof(u16), cube_indicies);
    
    copy_queue->ExecuteCommandLists(&copy_list, 1);
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "COLOR",    0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    D3D12_ROOT_PARAMETER1 root_params[Param_Count];
    root_params[Param_MatrixCB] = d3d::root_param1::InitAsConstant(sizeof(MatrixCB)/4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    
    // Diffuse texture sampler
    //D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 0, NULL, root_sig_flags);
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(g_cube_shaders[0]);
    shader_modules.pixel  = LoadShaderModule(g_cube_shaders[1]);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &_root_signature;
    pso_desc.shader_modules      = shader_modules;
    pso_desc.blend               = GetBlendState(BlendState::Disabled);
    pso_desc.depth               = GetDepthStencilState(DepthStencilState::ReadWrite);
    pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
    pso_desc.input_layouts       = &input_desc[0];
    pso_desc.input_layouts_count = _countof(input_desc);
    pso_desc.topology            = GfxTopology::Triangle;
    pso_desc.sample_desc.count   = g_msaa_sample_desc.Count;
    pso_desc.sample_desc.quality = g_msaa_sample_desc.Quality;
    pso_desc.dsv_format          = GfxFormat::D32_Float;
    pso_desc.rtv_formats.NumRenderTargets = 1;
    pso_desc.rtv_formats.RTFormats[0]     = g_back_buffer_format;
    _pso.Init(&pso_desc);
    
    _model = m4_translate(pos);
}

void 
experimental::Cube::Free()
{
    _vbuffer.Free();
    _ibuffer.Free();
    _pso.Free();
    _root_signature.Free();
}

void 
experimental::Cube::Render(CommandList *command_list, m4 proj_view)
{
    m4 mvp = m4_mul(proj_view, _model);
    
    // Set the graphics pipeline
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    
    MatrixCB cb = {};
    cb.matrix = mvp;
    command_list->SetGraphics32BitConstants(Param_MatrixCB, &cb);
    
    command_list->SetVertexBuffer(0, &_vbuffer);
    command_list->SetIndexBuffer(&_ibuffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->DrawIndexedInstanced((UINT)_ibuffer._count);
}

void 
experimental::EnvMapCube::Init(v3 pos)
{
    // Load vertex data!
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *copy_list = copy_queue->GetCommandList();
    
    CreateCube(copy_list, &_vbuffer, &_ibuffer);
    
    copy_queue->ExecuteCommandLists(&copy_list, 1);
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
    };
    
    D3D12_DESCRIPTOR_RANGE1 range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    
    // Diffuse texture sampler
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, 
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
                                                                         D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    
    D3D12_ROOT_PARAMETER1 root_params[Param_Count];
    root_params[Param_MatrixCB] = d3d::root_param1::InitAsConstant(sizeof(MatrixCB)/4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_params[Param_CameraCB] = d3d::root_param1::InitAsConstant(sizeof(v3)/4, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    root_params[Param_Skybox]   = d3d::root_param1::InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(g_env_mapping_shaders[0]);
    shader_modules.pixel  = LoadShaderModule(g_env_mapping_shaders[1]);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &_root_signature;
    pso_desc.shader_modules      = shader_modules;
    pso_desc.blend               = GetBlendState(BlendState::Disabled);
    pso_desc.depth               = GetDepthStencilState(DepthStencilState::ReadWrite);
    pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
    pso_desc.input_layouts       = &input_desc[0];
    pso_desc.input_layouts_count = _countof(input_desc);
    pso_desc.topology            = GfxTopology::Triangle;
    pso_desc.sample_desc.count   = g_msaa_sample_desc.Count;
    pso_desc.sample_desc.quality = g_msaa_sample_desc.Quality;
    pso_desc.dsv_format          = GfxFormat::D32_Float;
    pso_desc.rtv_formats.NumRenderTargets = 1;
    pso_desc.rtv_formats.RTFormats[0]     = g_back_buffer_format;
    _pso.Init(&pso_desc);
    
    _model = m4_translate(pos);
}

void 
experimental::EnvMapCube::Free()
{
}

void 
experimental::EnvMapCube::Render(CommandList *command_list, m4 proj_view)
{
    v3 pos = g_camera._position;
    MatrixCB cb = { proj_view, _model };
    
    // Set the graphics pipeline
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    
    command_list->SetGraphics32BitConstants(Param_MatrixCB, &cb);
    command_list->SetGraphics32BitConstants(Param_CameraCB, &pos);
    command_list->SetShaderResourceView(Param_Skybox, 0, &g_skybox._srv, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    command_list->SetVertexBuffer(0, &_vbuffer);
    command_list->SetIndexBuffer(&_ibuffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->DrawIndexedInstanced((UINT)_ibuffer._count);
}
