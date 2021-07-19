namespace environ_mapping
{
    struct TexturedCube
    {
        enum RootParams { Param_MatrixCB, Param_Diffuse, Param_Count };
        struct MatrixCB { m4 matrix; };
        
        Cube                _cube;
        TEXTURE_ID          _diffuse;
        m4                  _model;
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        
        void Init(CommandList *cpy_list, v3 pos = {0,0,0});
        void Free();
        void Render(CommandList *command_list, m4 proj_view);
    };
    
    struct EnvMapCube
    {
        enum RootParams { Param_MatrixCB, Param_CameraCB, Param_Skybox, Param_Count };
        struct MatrixCB { m4 vp; m4 model; };
        struct CameraCB { v3 cam; };
        
        Cube                _cube;
        m4                  _model;
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        
        void Init(CommandList *cpy_list, v3 pos = {0,0,0});
        void Free();
        void Render(CommandList *command_list, m4 proj_view);
    };
    
    struct Skybox
    {
        void Init(CommandList *cpy_list, const char *file);
        void Free();
        void Render(CommandList *command_list, m4 view_proj);
        
        RootSignature       _root_signature;
        PipelineStateObject _pso;
        ShaderResourceView  _srv;
        TEXTURE_ID          _cubemap;
        TEXTURE_ID          _texture;
        Cube                _cube;
    };
    
    static wchar_t *g_skybox_shaders[] = {
        L"shaders/Skybox_Vtx.cso",
        L"shaders/Skybox_Pxl.cso"
    };
    
    static wchar_t *g_env_mapping_shaders[] = {
        L"shaders/EnvMapping_Vtx.cso",
        L"shaders/EnvMapping_Pxl.cso"
    };
    
    static wchar_t *g_cube_shaders[] = {
        L"shaders/Cube_Vtx.cso",
        L"shaders/Cube_Pxl.cso"
    };
    
    // Is the scene currently active?
    static bool g_is_active = false;
    
    // Scene data
    ViewportCamera           g_camera;
    static TexturedCube      g_cube;
    static EnvMapCube        g_env_map_cube;
    static Skybox            g_skybox; 
    
    // Render Target
    static bool              g_render_msaa = true;
    static const DXGI_FORMAT g_back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    static DXGI_SAMPLE_DESC  g_msaa_sample_desc;
    static TEXTURE_ID        g_final_color_texture       = INVALID_TEXTURE_ID;
    static TEXTURE_ID        g_multisample_color_texture = INVALID_TEXTURE_ID;
    static TEXTURE_ID        g_depth_texture             = INVALID_TEXTURE_ID;
    static RenderTarget      g_render_target             = {};
    
    void OnInit(u32 width, u32 height);
    void OnRender(CommandList *command_list, v2 dims);
    void OnFree();
    ViewportCamera* GetViewportCamera();
    
    void OnDrawSceneData();
    void OnDrawSelectedObject();
};


void environ_mapping::OnInit(u32 width, u32 height)
{
    // Check the best multisample quality level that can be used for the given back buffer format.
    g_msaa_sample_desc = device::GetMultisampleQualityLevels(g_back_buffer_format);
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = copy_queue->GetCommandList();
    
    // Cube to render
    g_cube.Init(command_list);
    g_env_map_cube.Init(command_list, {5,0,0});
    
    //g_skybox.Init("textures/hdr/lilienstein_8k.hdr");
    //g_skybox.Init("textures/hdr/dikhololo_night_4k.hdr");
    //g_skybox.Init("textures/hdr/GCanyon_C_YumaPoint_Env.hdr");
    //g_skybox.Init("textures/hdr/Brooklyn_Bridge_Planks_Env.hdr");
    //g_skybox.Init("textures/hdr/Newport_Loft_Ref.hdr");
    g_skybox.Init(command_list, "textures/hdr/christmas_photo_studio_01_4k.hdr");
    
    copy_queue->ExecuteCommandLists(&command_list, 1);
    
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
    
    // Make sure the device is finished uploading resources
    copy_queue->Flush();
}

void environ_mapping::OnRender(CommandList *command_list, v2 dims)
{
    ImGuiIO& io = ImGui::GetIO();
    
    if (!g_is_active)
    {
        OnInit((u32)dims.x, (u32)dims.y);
        g_is_active = true;
    }
    
    // Update the camera (if it needs to)
    g_camera.OnUpdate(io.DeltaTime, { io.MouseDelta.x, io.MouseDelta.y });
    g_camera.OnMouseScroll(io.MouseWheel);
    
    g_render_target.Resize((u32)dims.x, (u32)dims.y);
    texture::Resize(g_final_color_texture, (u32)dims.x, (u32)dims.y);
    
    // Setup view projection matrix
    r32 aspect_ratio;
    if (dims.x >= dims.y)
    {
        aspect_ratio = (r32)dims.x / (r32)dims.y;
    }
    else 
    {
        aspect_ratio = (r32)dims.y / (r32)dims.x;
    }
    
    m4 projection_matrix = m4_perspective(g_camera._zoom, aspect_ratio, 0.1f, 100.0f);
    m4 view_matrix = g_camera.LookAt();
    
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
    
    
    TEXTURE_ID texture;
    if (g_render_msaa)
        texture = g_render_target.GetTexture(AttachmentPoint::Color0);
    else
        texture = g_final_color_texture;
    
    texture = g_final_color_texture;
    ImGui::Image((ImTextureID)((uptr)texture.val), 
                 ImVec2{ dims.x, dims.y }, 
                 ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}

void environ_mapping::OnDrawSceneData()
{
}

void environ_mapping::OnDrawSelectedObject()
{
}

void environ_mapping::OnFree()
{
    device::Flush();
    
    g_cube.Free();
    g_env_map_cube.Free();
    g_skybox.Free();
    
    texture::Free(g_final_color_texture);
    texture::Free(g_multisample_color_texture);
    texture::Free(g_depth_texture);
    g_render_target.Free();
    
    g_is_active = false;
}

ViewportCamera* environ_mapping::GetViewportCamera()
{
    return &g_camera;
}

void
environ_mapping::Skybox::Init(CommandList *copy_list, const char *file)
{
    _cube = CreateCube(copy_list, 5.0f, true);
    
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
environ_mapping::Skybox::Free()
{
    FreeCube(&_cube);
    _srv.Free();
    _pso.Free();
    _root_signature.Free();
    texture::Free(_cubemap);
    // TODO(Dustin): Queue this up for destruction with the command list
    // that created it...
    texture::Free(_texture);
}

void 
environ_mapping::Skybox::Render(CommandList *command_list, m4 view_proj)
{
    // Remove the transation part of the matrix
    //view_proj = m3_to_m4(m4_to_m3(view_proj));
    
    //m4 translate = m4_translate({0.5f, 0.5f, 0.f});
    //view_proj = m4_mul(translate, view_proj);
    
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    command_list->SetGraphics32BitConstants(0, &view_proj);
    command_list->SetShaderResourceView(1, 0, &_srv, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderCube(command_list, &_cube);
}

void 
environ_mapping::TexturedCube::Init(CommandList *copy_list, v3 pos)
{
    _cube = CreateCube(copy_list);
    _diffuse = copy_list->LoadTextureFromFile("textures/wall.jpg");
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    D3D12_DESCRIPTOR_RANGE1 texture_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    D3D12_ROOT_PARAMETER1 root_params[Param_Count];
    root_params[Param_MatrixCB] = d3d::root_param1::InitAsConstant(sizeof(MatrixCB)/4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_params[Param_Diffuse] = d3d::root_param1::InitAsDescriptorTable(1, &texture_range, D3D12_SHADER_VISIBILITY_PIXEL);
    
    // Diffuse texture sampler
    //D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
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
environ_mapping::TexturedCube::Free()
{
    FreeCube(&_cube);
    texture::Free(_diffuse);
    _pso.Free();
    _root_signature.Free();
}

void 
environ_mapping::TexturedCube::Render(CommandList *command_list, m4 proj_view)
{
    MatrixCB cb = { m4_mul(proj_view, _model) };
    
    // Set the graphics pipeline
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    command_list->SetGraphics32BitConstants(Param_MatrixCB, &cb);
    command_list->SetShaderResourceView(Param_Diffuse, 0, _diffuse, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderCube(command_list, &_cube);
}

void 
environ_mapping::EnvMapCube::Init(CommandList *copy_list, v3 pos)
{
    // Load vertex data!
    
    _cube = CreateCube(copy_list);
    
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
environ_mapping::EnvMapCube::Free()
{
    FreeCube(&_cube);
    _root_signature.Free();
    _pso.Free();
}

void 
environ_mapping::EnvMapCube::Render(CommandList *command_list, m4 proj_view)
{
    v3 pos = g_camera._position;
    MatrixCB cb = { proj_view, _model };
    
    // Set the graphics pipeline
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    
    command_list->SetGraphics32BitConstants(Param_MatrixCB, &cb);
    command_list->SetGraphics32BitConstants(Param_CameraCB, &pos);
    command_list->SetShaderResourceView(Param_Skybox, 0, &g_skybox._srv, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderCube(command_list, &_cube);
}