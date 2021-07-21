namespace HDR
{
    struct Tonemapper
    {
        u32   use_hdr;
        float exposure;
        float gamma;
    };
    
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
    
    namespace Tonemapper_RP
    {
        enum
        {
            TonemapperCB,
            HDRTexture,
            
            Count,
        };
    }
    
    static wchar_t *g_lighting_shaders[] = {
        L"shaders/HDRLighting_Vtx.cso",
        L"shaders/HDRLighting_Pxl.cso"
    };
    
    static wchar_t *g_light_cube_shaders[] = {
        L"shaders/LightCube_Vtx.cso",
        L"shaders/LightCube_Pxl.cso"
    };
    
    static wchar_t *g_hdr_shaders[] = {
        L"shaders/HDRToSDR_Vtx.cso",
        L"shaders/HDRToSDR_Pxl.cso"
    };
    
    // Scene Lights
    static LightProperties     g_light_prop = {};
    static bool                g_has_directional_light;
    static DirectionalLight    g_directional_light;
    static SpotLight           g_spot_lights[1];
    
    static PointLight         *g_point_lights = 0;
    static Cube                g_point_light_cube;
    
    // Scene Geometry
    static LitCube             g_cube;
    
    // Camera
    static ViewportCamera      g_camera;
    
    // Render Target information
    static bool                g_render_msaa = true;
    static DXGI_SAMPLE_DESC    g_msaa_sample_desc;
    static const DXGI_FORMAT   g_hdr_format                 = DXGI_FORMAT_R16G16B16A16_FLOAT;// HDR format
    static const DXGI_FORMAT   g_sdr_format                 = DXGI_FORMAT_R8G8B8A8_UNORM;    // SDR format
    static const DXGI_FORMAT   g_depth_format               = DXGI_FORMAT_D32_FLOAT;
    static TEXTURE_ID          g_hdr_resolved_texture       = INVALID_TEXTURE_ID;
    static TEXTURE_ID          g_hdr_texture                = INVALID_TEXTURE_ID;
    static TEXTURE_ID          g_sdr_texture                = INVALID_TEXTURE_ID;
    static TEXTURE_ID          g_depth_texture              = INVALID_TEXTURE_ID;
    static RenderTarget        g_hdr_render_target          = {};
    static RenderTarget        g_sdr_render_target          = {};
    
    // Pipeline information
    static RootSignature       g_hdr_signature;
    static RootSignature       g_sdr_signature;
    static RootSignature       g_light_cube_signature;
    static PipelineStateObject g_hdr_pso;
    static PipelineStateObject g_sdr_pso;
    static PipelineStateObject g_light_cube_pso;
    
    // Tonemapper
    static Tonemapper          g_tonemapper;
    
    // Show light cubes?
    static bool                g_show_debug_lights;
    
    static bool g_is_active = false;
    
    void OnInit(u32 width, u32 height);
    void OnRender(CommandList *command_list, v2 dims);
    void OnFree();
    ViewportCamera* GetViewportCamera();
    
    void OnDrawSceneData();
    void OnDrawSelectedObject();
};

void HDR::OnInit(u32 width, u32 height)
{
    
    //-------------------------------------------------------------------------------------------//
    // Load Scene Geometry
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = copy_queue->GetCommandList();
    
    g_cube.Init(command_list);
    
    // Set the cube's material
    Material mat = {};
    mat.ambient = V4_ZERO;
    mat.diffuse = V4_ONE;
    mat.specular = V4_ZERO;
    mat.has_diffuse = true; 
    mat.specular_power = 32.0f;
    g_cube._material = mat;
    
    // Set the cube's transform
    m4 model = m4_translate({ 0.0f, 0.0f, 25.0 });
    m4 scale = m4_scale(2.5f, 2.5f, 27.5f);
    g_cube._model = m4_mul(model, scale);
    
    // Point Lights!
    
    // Point lights
    {
        PointLight light = {};
        
        light.position_ws  = {  0.0f,  0.0f, 49.5f, 1.0f };
        light.color     = { 200.0f, 200.0f, 200.0f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws  = { 0.0f, -1.8f, 4.0f, 1.0f };
        light.color        = { 0.0f,   0.0f, 0.2f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws  = { 0.8f, -1.7f, 6.0f, 1.0f };
        light.color        = { 0.0f, 0.1f, 0.0f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        light = {};
        light.position_ws = { -1.4f, -1.9f, 9.0f, 1.0f };
        light.color       = { 0.1f, 0.0f, 0.0f, 1.0f };
        light.constant  = 1.0f;
        light.lin       = 0.09f;
        light.quadratic = 0.032f;
        arrput(g_point_lights, light);
        
        g_point_light_cube = CreateCube(command_list);
    }
    
    copy_queue->ExecuteCommandLists(&command_list, 1);
    
    //-------------------------------------------------------------------------------------------//
    // Create Render Targets + Textures
    
    // Check the best multisample quality level that can be used for the given back buffer format.
    g_msaa_sample_desc = device::GetMultisampleQualityLevels(g_hdr_format);
    
    D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(g_hdr_format, width, height, 1, 1, 
                                                           g_msaa_sample_desc.Count, g_msaa_sample_desc.Quality);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
    D3D12_CLEAR_VALUE color_clear_value;
    color_clear_value.Format = color_tex_desc.Format;
    color_clear_value.Color[0] = 0.0f;
    color_clear_value.Color[1] = 0.0f;
    color_clear_value.Color[2] = 0.0f;
    color_clear_value.Color[3] = 1.0f;
    
    g_hdr_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
    color_tex_desc = d3d::GetTex2DDesc(g_hdr_format, width, height, 1, 1);
    g_hdr_resolved_texture = texture::Create(&color_tex_desc, NULL);
    
    color_tex_desc = d3d::GetTex2DDesc(g_sdr_format, width, height, 1, 1);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    color_clear_value.Format = color_tex_desc.Format;
    g_sdr_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
    // Resize the depth texture.
    D3D12_RESOURCE_DESC depthTextureDesc = d3d::GetTex2DDesc(g_depth_format, width, height, 1, 1, 
                                                             g_msaa_sample_desc.Count, g_msaa_sample_desc.Quality);
    
    // Must be set on textures that will be used as a depth-stencil buffer.
    depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    // Specify optimized clear values for the depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil      = { 1.0F, 0 };
    g_depth_texture = texture::Create(&depthTextureDesc, &optimizedClearValue);
    
    g_hdr_render_target.Init();
    g_hdr_render_target.AttachTexture(AttachmentPoint::Color0, g_hdr_texture);
    g_hdr_render_target.AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
    
    g_sdr_render_target.Init();
    g_sdr_render_target.AttachTexture(AttachmentPoint::Color0, g_sdr_texture);
    
    //-------------------------------------------------------------------------------------------//
    // Camera
    
    // View matrix
    v3 eye_pos = { 0, -1.311f, -1.927f };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    g_camera.Init(eye_pos);
    
    //-------------------------------------------------------------------------------------------//
    // Create Lighting Root Signature
    
    {
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
        
        g_hdr_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    }
    
    //-------------------------------------------------------------------------------------------//
    // Create Lighting PSO
    
    {
        GfxInputElementDesc input_desc[] = {
            { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        };
        
        GfxShaderModules shader_modules;
        shader_modules.vertex = LoadShaderModule(g_lighting_shaders[0]);
        shader_modules.pixel  = LoadShaderModule(g_lighting_shaders[1]);
        
        GfxPipelineStateDesc pso_desc{};
        pso_desc.root_signature      = &g_hdr_signature;
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
        pso_desc.rtv_formats.RTFormats[0]     = g_hdr_format;
        g_hdr_pso.Init(&pso_desc);
        
        // Set default lighting mode to Phong lighting
        g_light_prop.UseBlinnPhong = true;
    }
    
    //-------------------------------------------------------------------------------------------//
    // Create HDR -> SDR RS & PSO
    
    {
        D3D12_ROOT_PARAMETER1 ptonemapper = d3d::root_param1::InitAsConstant(sizeof(Tonemapper)/4, 0, 0,
                                                                             D3D12_SHADER_VISIBILITY_PIXEL);
        
        D3D12_DESCRIPTOR_RANGE1 ranges[1] = {
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0), // diffuse texture
        };
        D3D12_ROOT_PARAMETER1 ptextures = d3d::root_param1::InitAsDescriptorTable(_countof(ranges), ranges,
                                                                                  D3D12_SHADER_VISIBILITY_PIXEL);
        
        D3D12_ROOT_PARAMETER1 root_params[Tonemapper_RP::Count];
        root_params[Tonemapper_RP::TonemapperCB] = ptonemapper;
        root_params[Tonemapper_RP::HDRTexture]   = ptextures;
        
        
        D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
        
        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        
        g_sdr_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    }
    
    {
        GfxShaderModules shader_modules;
        shader_modules.vertex = LoadShaderModule(g_hdr_shaders[0]);
        shader_modules.pixel  = LoadShaderModule(g_hdr_shaders[1]);
        
        GfxPipelineStateDesc pso_desc{};
        pso_desc.root_signature      = &g_sdr_signature;
        pso_desc.shader_modules      = shader_modules;
        pso_desc.blend               = GetBlendState(BlendState::Disabled);
        pso_desc.depth               = GetDepthStencilState(DepthStencilState::Disabled);
        
        pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
        pso_desc.rasterizer.DepthClipEnable = false;
        
        pso_desc.input_layouts       = NULL;
        pso_desc.input_layouts_count = 0;
        pso_desc.topology            = GfxTopology::Triangle;
        pso_desc.sample_desc.count   = 1;
        
        pso_desc.rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.rtv_formats.NumRenderTargets = 1;
        
        g_sdr_pso.Init(&pso_desc);
    }
    
    //-------------------------------------------------------------------------------------------//
    // Create Light Cube RS & PSO
    
    {
        D3D12_ROOT_PARAMETER1 ptonemapper = d3d::root_param1::InitAsConstant(sizeof(Tonemapper)/4, 0, 0,
                                                                             D3D12_SHADER_VISIBILITY_PIXEL);
        
        D3D12_ROOT_PARAMETER1 root_params[1];
        root_params[0] = d3d::root_param1::InitAsConstant(sizeof(m4)/4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        
        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        
        g_light_cube_signature.Init(_countof(root_params), root_params, 0, NULL, root_sig_flags);
    }
    
    {
        GfxShaderModules shader_modules;
        shader_modules.vertex = LoadShaderModule(g_light_cube_shaders[0]);
        shader_modules.pixel  = LoadShaderModule(g_light_cube_shaders[1]);
        
        GfxInputElementDesc input_desc[] = {
            { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
        };
        
        GfxPipelineStateDesc pso_desc{};
        pso_desc.root_signature      = &g_light_cube_signature;
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
        pso_desc.rtv_formats.RTFormats[0]     = g_hdr_format;
        g_light_cube_pso.Init(&pso_desc);
    }
    
    //-------------------------------------------------------------------------------------------//
    // Tonemaper
    
    g_tonemapper.use_hdr = false;
    g_tonemapper.exposure = 7.5f;
    g_tonemapper.gamma = 1.1f;
    
    // Make sure resources have been uploaded to GPU before
    // rendering the first frame.
    device::Flush();
}

void HDR::OnRender(CommandList *command_list, v2 dims)
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
    
    g_hdr_render_target.Resize((u32)dims.x, (u32)dims.y);
    g_sdr_render_target.Resize((u32)dims.x, (u32)dims.y);
    texture::Resize(g_hdr_resolved_texture, (u32)dims.x, (u32)dims.y);
    
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
    
#if 0
    for (u32 i = 0; i < _countof(g_spot_lights); ++i)
    {
        g_spot_lights[i].position_vs = m4_mul_v4(view_matrix, g_spot_lights[i].position_ws);
        g_spot_lights[i].direction_vs = m4_mul_v4(view_matrix, g_spot_lights[i].direction_ws);
    }
#endif
    
    D3D12_VIEWPORT viewport = g_hdr_render_target.GetViewport();
    command_list->SetViewport(viewport);
    
    D3D12_RECT scissor_rect = {};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)viewport.Width;
    scissor_rect.bottom = (LONG)viewport.Height;
    command_list->SetScissorRect(scissor_rect);
    
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list->ClearTexture(g_hdr_render_target.GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->ClearDepthStencilTexture(g_hdr_render_target.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    
    command_list->SetRenderTarget(&g_hdr_render_target);
    
    // Render
    
    command_list->SetPipelineState(g_hdr_pso._handle);
    command_list->SetGraphicsRootSignature(&g_hdr_signature);
    
    g_light_prop.HasDirectionalLight = false; //g_has_directional_light;
    g_light_prop.NumPointLights = (u32)arrlen(g_point_lights);
    g_light_prop.NumSpotLights = 0; // _countof(g_spot_lights);
    g_light_prop.HasGammaCorrection = true;
    //g_light_prop.CameraPos = g_camera._position;
    
    command_list->SetGraphics32BitConstants(LitCube_RP::LightPropCB, &g_light_prop);
    
    //if (g_light_prop.NumPointLights > 0)
    command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::PointLightSB, arrlen(g_point_lights), 
                                                     sizeof(g_point_lights[0]), g_point_lights);
    
    //if (g_light_prop.NumSpotLights > 0)
    command_list->SetGraphicsDynamicStructuredBuffer(LitCube_RP::SpotLightSB, _countof(g_spot_lights), 
                                                     sizeof(g_spot_lights[0]), g_spot_lights);
    
    //if (g_light_prop.HasDirectionalLight)
    command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::DirLightCB, &g_directional_light);
    
    // Draw scene geometry!
    command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::MaterialCB, &g_cube._material);
    
    m4 proj_view = m4_mul(projection_matrix, view_matrix);
    Mat_CB matrix = {};
    matrix.Model = g_cube._model;
    matrix.ModelView = m4_mul(view_matrix, matrix.Model);
    matrix.TransposeModelView = m4_transpose(m4_inverse(matrix.ModelView));
    matrix.ModelViewProjection = m4_mul(proj_view, matrix.Model);
    command_list->SetGraphicsDynamicConstantBuffer(LitCube_RP::MatrixCB, &matrix);
    
    g_cube.Render(command_list);
    
    if (g_show_debug_lights)
    {
        // Visualize lights in the scene
        command_list->SetPipelineState(g_light_cube_pso._handle);
        command_list->SetGraphicsRootSignature(&g_light_cube_signature);
        
        for (u32 i = 0; i < arrlen(g_point_lights); ++i)
        {
            m4 model = m4_translate(g_point_lights[i].position_ws.xyz);
            model = m4_mul(model, m4_scale(0.1f,0.1f,0.1f));
            
            m4 mvp = m4_mul(proj_view, model);
            command_list->SetGraphics32BitConstants(0, &mvp);
            
            RenderCube(command_list, &g_point_light_cube);
        }
    }
    
    // Resolve the multisampled render target into an intermediary texture
    if (g_msaa_sample_desc.Count > 1)
    {
        Resource *src_texture = texture::GetResource(g_hdr_render_target.GetTexture(AttachmentPoint::Color0));
        Resource *dst_texture = texture::GetResource(g_hdr_resolved_texture);
        command_list->ResolveSubresource(dst_texture, src_texture);
    }
    else
    {
        Resource *src_texture = texture::GetResource(g_hdr_render_target.GetTexture(AttachmentPoint::Color0));
        Resource *dst_texture = texture::GetResource(g_hdr_resolved_texture);
        command_list->CopyResource(dst_texture, src_texture);
    }
    
    // Convert HDR -> SDR
    command_list->ClearTexture(g_sdr_render_target.GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->SetRenderTarget(&g_sdr_render_target);
    
    command_list->SetPipelineState(g_sdr_pso._handle);
    command_list->SetGraphicsRootSignature(&g_sdr_signature);
    command_list->SetGraphics32BitConstants(Tonemapper_RP::TonemapperCB, &g_tonemapper);
    command_list->SetShaderResourceView(Tonemapper_RP::HDRTexture, 0, g_hdr_resolved_texture);
    command_list->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    command_list->DrawInstanced(4);
    
    // Draw to the ImGui viewport
    TEXTURE_ID texture = g_sdr_render_target.GetTexture(AttachmentPoint::Color0);
#if 1
    ImGui::Image((ImTextureID)((uptr)texture.val),
                 ImVec2{ dims.x, dims.y }, 
                 ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
#else
    ImGui::Image((ImTextureID)((uptr)texture.val),
                 ImVec2{ dims.x, dims.y }, 
                 ImVec2{ 1, 0 }, ImVec2{ 0, 1 });
#endif
}

void HDR::OnFree()
{
    device::Flush();
    
    arrfree(g_point_lights);
    FreeCube(&g_point_light_cube);
    g_cube.Free();
    
    
    // Pipeline information
    g_hdr_signature.Free();
    g_sdr_signature.Free();
    g_light_cube_signature.Free();
    g_hdr_pso.Free();
    g_sdr_pso.Free();
    g_light_cube_pso.Free();
    
    texture::Free(g_hdr_resolved_texture);
    texture::Free(g_hdr_texture);
    texture::Free(g_sdr_texture);
    texture::Free(g_depth_texture);
    g_hdr_render_target.Free();
    g_sdr_render_target.Free();
    
    g_is_active = false;
}

ViewportCamera* HDR::GetViewportCamera()
{
    return &g_camera;
}

void HDR::OnDrawSceneData()
{
    if (ImGui::Checkbox("Use HDR", (bool*)&g_tonemapper.use_hdr)) {}
    if (ImGui::Checkbox("Debug Lights", (bool*)&g_show_debug_lights)) {}
    ImGui::InputFloat("Gamma", &g_tonemapper.gamma, 0.1f);
    ImGui::InputFloat("Exposure", &g_tonemapper.exposure, 0.1f);
}

void HDR::OnDrawSelectedObject()
{
}


void 
HDR::LitCube::Init(CommandList *copy_list)
{
    _cube = CreateCube(copy_list, 1.0f, false, false);
    _diffuse = copy_list->LoadTextureFromFile("textures/planks/BaseColor.png", true, true);
}

void 
HDR::LitCube::Render(CommandList *command_list)
{
    // Set the graphics pipeline
    command_list->SetShaderResourceView(LitCube_RP::Textures, 0, _diffuse, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderCube(command_list, &_cube);
}

void 
HDR::LitCube::Free()
{
    FreeCube(&_cube);
    texture::Free(_diffuse);
}
