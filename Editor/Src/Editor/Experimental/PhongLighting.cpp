
// TODO(Dustin): 
// - Blinn-Phong
// - In-editor movement of lights

namespace phong_lighting
{
    
    struct LitCube
    {
        void Init(CommandList *cpy_list);
        void Free();
        void Render(CommandList *command_list);
        
        Cube                _cube;
        Material            _material;
        TEXTURE_ID          _diffuse;
        m4                  _model;
    };
    
    namespace LitCube_RP
    {
        enum
        {
            MatrixCB,        // VS: register(b0, space0)
            MaterialCB,      // PS: register(b0, space1)
            LightPropCB,     // PS: register(b1, space0)
            DirLightCB,      // PS: register(b2, space0)
            PointLightSB,    // PS: register(t0, space0)
            SpotLightSB,     // PS: register(t1, space0)
            Textures,        // PS: register(t2, space0)
            
            Count,
        };
    };
    
    static wchar_t *g_lighting_shaders[] = {
        L"shaders/PhongLighting_Vtx.cso",
        L"shaders/PhongLighting_Pxl.cso"
    };
    
    static LightProperties     g_light_prop = {};
    
    static RootSignature       g_lighting_signature;
    static PipelineStateObject g_lighting_pso;
    
    static bool                g_has_directional_light;
    static DirectionalLight    g_directional_light;
    static PointLight         *g_point_lights = 0;
    static SpotLight          *g_spot_lights = 0;
    
    static ViewportCamera      g_camera;
    
    // Scene Geometry
    
    v3 g_cube_positions[] = {
        { 0.0f,  0.0f,  0.0f  },
        { 2.0f,  5.0f, -15.0f },
        {-1.5f, -2.2f, -2.5f  },
        {-3.8f, -2.0f, -12.3f },
        { 2.4f, -0.4f, -3.5f  },
        {-1.7f,  3.0f, -7.5f  },
        { 1.3f, -2.0f, -2.5f  },
        { 1.5f,  2.0f, -2.5f  },
        { 1.5f,  0.2f, -1.5f  },
        {-1.3f,  1.0f, -1.5f  }
    };
    
    v4 g_point_light_positions[] = {
        { 0.7f,  0.2f,  2.0f, 1.0f  },
        { 2.3f, -3.3f, -4.0f, 1.0f  },
        {-4.0f,  2.0f, -12.0f, 1.0f },
        { 0.0f,  0.0f, -3.0f, 1.0f  }
    };
    
    static LitCube             g_demo_cubes[10];
    
    // Render Target
    static bool                g_render_msaa = true;
    static const DXGI_FORMAT   g_back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM;//_SRGB
    static DXGI_SAMPLE_DESC    g_msaa_sample_desc;
    static TEXTURE_ID          g_final_color_texture       = INVALID_TEXTURE_ID;
    static TEXTURE_ID          g_multisample_color_texture = INVALID_TEXTURE_ID;
    static TEXTURE_ID          g_depth_texture             = INVALID_TEXTURE_ID;
    static RenderTarget        g_render_target             = {};
    
    // Is the scene currently active?
    static bool                g_is_active = false;
    
    void OnInit(u32 width, u32 height);
    void OnRender(CommandList *command_list, v2 dims);
    void OnFree();
    ViewportCamera* GetViewportCamera();
    
    void OnDrawSceneData();
    void OnDrawSelectedObject();
};


void phong_lighting::OnInit(u32 width, u32 height)
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
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = copy_queue->GetCommandList();
    
    //-------------------------------------------------------------------------------------------//
    // Load Scene Geometry
    
    // Load CUBES
    
    for (u32 i = 0; i < _countof(g_cube_positions); ++i)
    {
        g_demo_cubes[i].Init(command_list);
        
        // Set the cube's material
        Material mat = {};
        mat.ambient = V4_ZERO;
        mat.diffuse = V4_ONE;
        mat.specular = V4_ONE;
        mat.has_diffuse = true; 
        mat.specular_power = 8.0f;
        g_demo_cubes[i]._material = mat;
        
        // Set the cube's transform
        m4 model = m4_translate(g_cube_positions[i]);
        m4 rotation = m4_rotate(20.0f * i, {1.0f, 0.3f, 0.5f });
        g_demo_cubes[i]._model = m4_mul(rotation, model);
    }
    
    // Directional Light
    {
        g_has_directional_light = true;
        g_directional_light.direction_ws = { -0.2f, -1.0f, -0.3f };
        g_directional_light.color = { 0.4f, 0.4f, 0.4f, 1.0f };
    }
    
    // Point lights
    {
        PointLight light = {};
        
        light.position_ws  = g_point_light_positions[0];
        light.color     = { 0.8f, 0.8f, 0.8f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws  = g_point_light_positions[1];
        light.color     = { 0.8f, 0.8f, 0.8f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws  = g_point_light_positions[2];
        light.color     = { 0.8f, 0.8f, 0.8f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws  = g_point_light_positions[3];
        light.color     = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
    }
    
    // Spot light
    if (0) {
        SpotLight light = {};
        
        light.position_ws.xyz = g_camera._position;
        light.position_ws.w   = 1.0f;
        
        light.direction_ws.xyz = g_camera._front;
        light.direction_ws.w   = 1.0f;
        
        light.color        = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.cutoff       = cosf(degrees_to_radians(12.5f));
        light.outer_cutoff = cosf(degrees_to_radians(15.0f));
        light.constant     = 1.0f;
        light.lin          = 0.09f;
        light.quadratic    = 0.032f;
        
        arrput(g_spot_lights, light);
    }
    
    // TODO(Dustin): Example cubes for lights!
    
    copy_queue->ExecuteCommandLists(&command_list, 1);
    
    //-------------------------------------------------------------------------------------------//
    // Create Lighting Root Signature
    
    // matrix root descriptor
    D3D12_ROOT_PARAMETER1 pmatrix = d3d::root_param1::InitAsConstantBufferView(0, 0, 
                                                                               D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                               D3D12_SHADER_VISIBILITY_VERTEX);
    // material root descriptor
    D3D12_ROOT_PARAMETER1 pmaterial = d3d::root_param1::InitAsConstantBufferView(0, 1, 
                                                                                 D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                                 D3D12_SHADER_VISIBILITY_PIXEL);
    // light properties root descriptor
    D3D12_ROOT_PARAMETER1 plight_prop = d3d::root_param1::InitAsConstant(sizeof(LightProperties)/4, 1, 0,
                                                                         D3D12_SHADER_VISIBILITY_PIXEL);
    // directional light root descriptor
    D3D12_ROOT_PARAMETER1 pdirectional = d3d::root_param1::InitAsConstantBufferView(2, 0, 
                                                                                    D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                                    D3D12_SHADER_VISIBILITY_PIXEL);
    // point light root descriptor
    D3D12_ROOT_PARAMETER1 ppoint = d3d::root_param1::InitAsShaderResourceView(0, 0, 
                                                                              D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                              D3D12_SHADER_VISIBILITY_PIXEL);
    // spot light root descriptor
    D3D12_ROOT_PARAMETER1 pspot = d3d::root_param1::InitAsShaderResourceView(1, 0, 
                                                                             D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                             D3D12_SHADER_VISIBILITY_PIXEL);
    
    // Texture(s) descriptor table
    D3D12_DESCRIPTOR_RANGE1 ranges[1] = {
        d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2), // diffuse texture
    };
    D3D12_ROOT_PARAMETER1 ptextures = d3d::root_param1::InitAsDescriptorTable(_countof(ranges), ranges,
                                                                              D3D12_SHADER_VISIBILITY_PIXEL);
    
    D3D12_ROOT_PARAMETER1 root_params[LitCube_RP::Count];
    root_params[LitCube_RP::MatrixCB]     = pmatrix;
    root_params[LitCube_RP::MaterialCB]   = pmaterial;
    root_params[LitCube_RP::LightPropCB]  = plight_prop;
    root_params[LitCube_RP::DirLightCB]   = pdirectional;
    root_params[LitCube_RP::PointLightSB] = ppoint;
    root_params[LitCube_RP::SpotLightSB]  = pspot;
    root_params[LitCube_RP::Textures]     = ptextures;
    
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    g_lighting_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    //-------------------------------------------------------------------------------------------//
    // Create Lighting PSO
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(g_lighting_shaders[0]);
    shader_modules.pixel  = LoadShaderModule(g_lighting_shaders[1]);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &g_lighting_signature;
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
    g_lighting_pso.Init(&pso_desc);
    
    // Set default lighting mode to Phong lighting
    g_light_prop.UseBlinnPhong = false;
    
    device::Flush();
}

void phong_lighting::OnRender(CommandList *command_list, v2 dims)
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
    
    // Update lighting information so that it is in view space
    if (g_has_directional_light)
    {
        g_directional_light.direction_vs = m4_mul_v4(view_matrix, g_directional_light.direction_ws);
    }
    
    for (u32 i = 0; i < arrlen(g_point_lights); ++i)
    {
        g_point_lights[i].position_vs = m4_mul_v4(view_matrix, g_point_lights[i].position_ws);
    }
    
    for (u32 i = 0; i < arrlen(g_spot_lights); ++i)
    {
        g_spot_lights[i].position_vs = m4_mul_v4(view_matrix, g_spot_lights[i].position_ws);
        g_spot_lights[i].direction_vs = m4_mul_v4(view_matrix, g_spot_lights[i].direction_ws);
    }
    
    D3D12_VIEWPORT viewport = g_render_target.GetViewport();
    command_list->SetViewport(viewport);
    
    D3D12_RECT scissor_rect = {};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)viewport.Width;
    scissor_rect.bottom = (LONG)viewport.Height;
    command_list->SetScissorRect(scissor_rect);
    
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list->ClearTexture(g_render_target.GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->ClearDepthStencilTexture(g_render_target.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    
    command_list->SetRenderTarget(&g_render_target);
    
    // Render
    
    command_list->SetPipelineState(g_lighting_pso._handle);
    command_list->SetGraphicsRootSignature(&g_lighting_signature);
    
    g_light_prop.HasDirectionalLight = g_has_directional_light;
    g_light_prop.NumPointLights = (u32)arrlen(g_point_lights);
    g_light_prop.NumSpotLights = (u32)arrlen(g_spot_lights);
    g_light_prop.HasGammaCorrection = false;
    //g_light_prop.CameraPos = g_camera._position;
    
    command_list->SetGraphics32BitConstants(LitCube_RP::LightPropCB, &g_light_prop);
    
    u64 s = sizeof(LightProperties);
    
    //command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::PointLightSB, &g_point_lights, _countof(g_point_lights));
    command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::PointLightSB, 
                                                     arrlen(g_point_lights), 
                                                     sizeof(g_point_lights[0]), 
                                                     g_point_lights);
    
    //command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::SpotLightSB, &g_spot_lights, _countof(g_spot_lights));
    command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::SpotLightSB, 
                                                     arrlen(g_spot_lights), 
                                                     sizeof(g_spot_lights[0]), 
                                                     g_spot_lights);
    
    
    command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::DirLightCB, &g_directional_light);
    
    // for each cube...
    command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::MaterialCB, &g_demo_cubes[0]._material);
    
    m4 proj_view = m4_mul(projection_matrix, view_matrix);;
    for (u32 i = 0; i < _countof(g_demo_cubes); ++i)
    {
        LitCube *cube = g_demo_cubes + i;
        
        Mat_CB matrix = {};
        matrix.Model = cube->_model;
        matrix.ModelView = m4_mul(view_matrix, matrix.Model);
        matrix.TransposeModelView = m4_transpose(matrix.ModelView);
        matrix.ModelViewProjection = m4_mul(proj_view, matrix.Model);
        
        command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::MatrixCB, &matrix);
        cube->Render(command_list);
    }
    
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
    
    TEXTURE_ID texture = g_final_color_texture;
    ImGui::Image((ImTextureID)((uptr)texture.val), 
                 ImVec2{ dims.x, dims.y }, 
                 ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}

void phong_lighting::OnDrawSceneData()
{
    if (ImGui::Checkbox("Use Blinn Phong", (bool*)&g_light_prop.UseBlinnPhong))
    {
    }
}

void phong_lighting::OnDrawSelectedObject()
{
}

void phong_lighting::OnFree()
{
    device::Flush();
    
    for (u32 i = 0; i < _countof(g_demo_cubes); ++i)
        g_demo_cubes[i].Free();
    
    texture::Free(g_final_color_texture);
    texture::Free(g_multisample_color_texture);
    texture::Free(g_depth_texture);
    g_render_target.Free();
    
    g_lighting_signature.Free();
    g_lighting_pso.Free();
    
    arrfree(g_point_lights);
    arrfree(g_spot_lights);
    
    g_is_active = false;
}

ViewportCamera* phong_lighting::GetViewportCamera()
{
    return &g_camera;
}

void 
phong_lighting::LitCube::Init(CommandList *copy_list)
{
    _cube = CreateCube(copy_list);
    _diffuse = copy_list->LoadTextureFromFile("textures/wall.jpg");
}

void 
phong_lighting::LitCube::Render(CommandList *command_list)
{
    // Set the graphics pipeline
    command_list->SetShaderResourceView(LitCube_RP::Textures, 0, _diffuse, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderCube(command_list, &_cube);
}

void 
phong_lighting::LitCube::Free()
{
    FreeCube(&_cube);
    texture::Free(_diffuse);
}
