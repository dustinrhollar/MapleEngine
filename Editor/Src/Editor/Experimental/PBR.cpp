
namespace PBR
{
    namespace PBR_RP
    {
        enum
        {
            MatrixCB,        // VS: register(b0, space0)
            MaterialCB,      // PS: register(b0, space1)
            LightPropCB,     // PS: register(b1, space0)
            //DirLightCB,      // PS: register(b2, space0)
            PointLightSB,    // PS: register(t0, space0)
            //SpotLightSB,     // PS: register(t1, space0)
            Textures,        // PS: register(t2 - t6, space0)
            
            Count,
        };
    };
    
    namespace TextureMap
    {
        enum
        {
            Albedo    = BIT(0),
            Normal    = BIT(1),
            Metallic  = BIT(2),
            Roughness = BIT(3),
            AO        = BIT(4),
        };
    };
    
    struct Mat_CB
    {
        m4 Model;
        m4 ModelView;
        m4 InverseTransposeModel;
        m4 ModelViewProjection;
    };
    
    struct Material
    {
        v4 ambient;
        //------------------------ 16 byte boundary
        v4 albedo;
        //------------------------ 16 bytes boundary
        v4 specular;
        //------------------------ 16 bytes boundary
        r32  metallic;
        r32  roughness;
        r32  ao;
        r32  f0; // surface reflection at zero incidence (IoR)
        //------------------------ 16 bytes boundary
        // NOTE(Dustin): As I integrate more texture maps into the shader,
        // could use a bitmask instead of a series of boolean values 
        u32  has_texture = 0; 
        v3   pad1;
        //------------------------ 16 bytes boundary
        //------------------------ Total: 80 bytes
    };
    
    struct LightProperties
    {
        v3  CameraPos;
        r32 pad0;
        u32 NumPointLights;
        u32 NumSpotLights;
    };
    
    struct PointLight
    {
        v4 position_ws;
        //------------------------ 16 byte boundary
        v4 color;
        //------------------------ 16 byte boundary
        //------------------------ Total: 32 bytes
    };
    
    static wchar_t *g_pbr_shaders[] = {
        L"shaders/PBR_Vtx.cso",
        L"shaders/PBR_Pxl.cso"
    };
    
#if 0
    static const char* g_albedo_file    = "textures/SeasideRocks/SeasideRocks01_4K_BaseColor.png";
    static const char* g_normal_file    = "textures/SeasideRocks/SeasideRocks01_4K_Normal.png";
    static const char* g_metallic_file  = "textures/SeasideRocks/SeasideRocks01_4K_Height.png";
    static const char* g_roughness_file = "textures/SeasideRocks/SeasideRocks01_4K_Roughness.png";
    static const char* g_ao_file        = "textures/SeasideRocks/SeasideRocks01_4K_AO.png";
#else
    static const char* g_albedo_file    = "textures/RustyMetal/RustyMetalPanel01_4K_BaseColor.png";
    static const char* g_normal_file    = "textures/RustyMetal/RustyMetalPanel01_4K_Normal.png";
    static const char* g_metallic_file  = "textures/RustyMetal/RustyMetalPanel01_4K_Height.png";
    static const char* g_roughness_file = "textures/RustyMetal/RustyMetalPanel01_4K_Roughness.png";
    static const char* g_ao_file        = "textures/RustyMetal/RustyMetalPanel01_4K_AO.png";
#endif
    
    static TEXTURE_ID g_albedo    = INVALID_TEXTURE_ID;
    static TEXTURE_ID g_normal    = INVALID_TEXTURE_ID;
    static TEXTURE_ID g_metallic  = INVALID_TEXTURE_ID;
    static TEXTURE_ID g_roughness = INVALID_TEXTURE_ID;
    static TEXTURE_ID g_ao        = INVALID_TEXTURE_ID;
    
    static ShaderResourceView g_null_srv = {};
    
    // Scene Geometry
    
    static i32                g_nr_rows = 1;
    static i32                g_nr_cols = 1;
    static r32                g_sphere_spacing = 2.5;
    
    Sphere                   *g_spheres = 0;
    m4                       *g_models = 0;
    Material                 *g_materials = 0;
    LightProperties           g_light_prop = {};
    PointLight               *g_point_lights = 0;
    
    RootSignature             g_pbr_signature;
    PipelineStateObject       g_pbr_pso;
    
    // Render Target
    static bool               g_render_msaa = true;
    static DXGI_SAMPLE_DESC   g_msaa_sample_desc;
    static const DXGI_FORMAT  g_back_buffer_format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    static TEXTURE_ID         g_final_color_texture       = INVALID_TEXTURE_ID;
    static TEXTURE_ID         g_msaa_color_texture        = INVALID_TEXTURE_ID;
    static TEXTURE_ID         g_depth_texture             = INVALID_TEXTURE_ID;
    static RenderTarget       g_render_target             = {};
    
    ViewportCamera g_camera;
    
    static bool g_is_active = false;
    
    void OnInit(u32 width, u32 height);
    void OnRender(CommandList *command_list, v2 dims);
    void OnFree();
    ViewportCamera* GetViewportCamera();
    
    void OnDrawSceneData();
    void OnDrawSelectedObject();
};

void PBR::OnInit(u32 width, u32 height)
{
    // Check the best multisample quality level that can be used for the given back buffer format.
    g_msaa_sample_desc = device::GetMultisampleQualityLevels(g_back_buffer_format);
    
    //-------------------------------------------------------------------------------------------//
    // Load Scene Geometry
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = copy_queue->GetCommandList();
    
#if 0
    v4 light_positions[] = {
        {-10.0f,  10.0f, 10.0f, 1.0f },
        { 10.0f,  10.0f, 10.0f, 1.0f },
        {-10.0f, -10.0f, 10.0f, 1.0f },
        { 10.0f, -10.0f, 10.0f, 1.0f },
    };
    
    v4 light_colors[] = {
        {300.0f, 300.0f, 300.0f, 1.0f },
        {300.0f, 300.0f, 300.0f, 1.0f },
        {300.0f, 300.0f, 300.0f, 1.0f },
        {300.0f, 300.0f, 300.0f, 1.0f },
    };
#else
    v4 light_positions[] = {
        {0.0f,  0.0f, 10.0f, 1.0f },
    };
    
    v4 light_colors[] = {
        {150.0f, 150.0f, 150.0f, 1.0f },
    };
#endif
    
    i32 count = g_nr_rows * g_nr_cols;
    arrsetcap(g_spheres, count);
    arrsetcap(g_materials, count);
    arrsetcap(g_models, count);
    
    // render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
    for (int row = 0; row < g_nr_rows; ++row) 
    {
        Material material = {};
        material.albedo = { 0.5f, 0.0f, 0.0f, 1.0f };
        material.ao = 1.0f;
        material.metallic = (r32)row / (r32)g_nr_rows;
        material.f0 = 0.04f; // surface reflection at zero incidence (IoR)
        material.has_texture = 
            TextureMap::Albedo|
            TextureMap::Normal|
            //TextureMap::Metallic|
            TextureMap::Roughness|
            TextureMap::AO;
        
        for (int col = 0; col < g_nr_cols; ++col) 
        {
            // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            //material.roughness = fast_clampf(0.05f, 1.0f, (r32)col / (r32)g_nr_cols);
            material.roughness = 1.0f;
            
            m4 model = m4_translate({
                                        (col - (g_nr_cols / 2)) * g_sphere_spacing, 
                                        (row - (g_nr_rows / 2)) * g_sphere_spacing, 
                                        0.0f
                                    });
            
            arrput(g_spheres, CreateSphere(command_list, 1.0f, 128));
            arrput(g_materials, material);
            arrput(g_models, model);
        }
    }
    
    // Load textures
    g_albedo    = command_list->LoadTextureFromFile(g_albedo_file,    true, true);
    g_normal    = command_list->LoadTextureFromFile(g_normal_file,    true, true);
    g_metallic  = command_list->LoadTextureFromFile(g_metallic_file,  true, true);
    g_roughness = command_list->LoadTextureFromFile(g_roughness_file, true, true);
    g_ao        = command_list->LoadTextureFromFile(g_ao_file,        true, true);
    
    copy_queue->ExecuteCommandLists(&command_list, 1);
    
    // Create the descriptors for each of the textures
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    g_null_srv.Init(NULL, &srv_desc);
    
    arrsetcap(g_point_lights, 4);
    for (u32 i = 0; i < _countof(light_positions); ++i)
    {
        PointLight light = {};
        light.position_ws = light_positions[i];
        light.color = light_colors[i];
        arrput(g_point_lights, light);
    }
    
    //-------------------------------------------------------------------------------------------//
    // Create PSO & Root Signature
    
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
        
        // point light root descriptor
        D3D12_ROOT_PARAMETER1 ppoint = d3d::root_param1::InitAsShaderResourceView(0, 0, 
                                                                                  D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
                                                                                  D3D12_SHADER_VISIBILITY_PIXEL);
        
        // Texture(s) descriptor table
        D3D12_DESCRIPTOR_RANGE1 ranges[5] = {
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2), // Albedo texture
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3), // Normal texture
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4), // Metallic texture
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5), // Roughness texture
            d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6), // AO texture
        };
        D3D12_ROOT_PARAMETER1 ptextures = d3d::root_param1::InitAsDescriptorTable(_countof(ranges), ranges,
                                                                                  D3D12_SHADER_VISIBILITY_PIXEL);
        
        D3D12_ROOT_PARAMETER1 root_params[PBR_RP::Count];
        root_params[PBR_RP::MatrixCB]     = pmatrix;
        root_params[PBR_RP::MaterialCB]   = pmaterial;
        root_params[PBR_RP::LightPropCB]  = plight_prop;
        //root_params[PBR_RP::DirLightCB]   = pdirectional;
        root_params[PBR_RP::PointLightSB] = ppoint;
        //root_params[PBR_RP::SpotLightSB]  = pspot;
        root_params[PBR_RP::Textures]     = ptextures;
        
        D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
        
        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        
        g_pbr_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    }
    
    {
        GfxInputElementDesc input_desc[] = {
            { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
            { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        };
        
        GfxShaderModules shader_modules;
        shader_modules.vertex = LoadShaderModule(g_pbr_shaders[0]);
        shader_modules.pixel  = LoadShaderModule(g_pbr_shaders[1]);
        
        GfxPipelineStateDesc pso_desc{};
        pso_desc.root_signature      = &g_pbr_signature;
        pso_desc.shader_modules      = shader_modules;
        pso_desc.blend               = GetBlendState(BlendState::Disabled);
        pso_desc.depth               = GetDepthStencilState(DepthStencilState::ReadWrite);
        
        pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
        //pso_desc.rasterizer.MultisampleEnable = TRUE;
        
        pso_desc.input_layouts       = &input_desc[0];
        pso_desc.input_layouts_count = _countof(input_desc);
        pso_desc.topology            = GfxTopology::Triangle;
        pso_desc.sample_desc.count   = g_msaa_sample_desc.Count;
        pso_desc.sample_desc.quality = g_msaa_sample_desc.Quality;
        pso_desc.dsv_format          = GfxFormat::D32_Float;
        pso_desc.rtv_formats.NumRenderTargets = 1;
        pso_desc.rtv_formats.RTFormats[0]     = g_back_buffer_format;
        g_pbr_pso.Init(&pso_desc);
    }
    
    
    //-------------------------------------------------------------------------------------------//
    // Create Render Target
    
    D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(g_back_buffer_format, width, height, 1, 1, 
                                                           g_msaa_sample_desc.Count, g_msaa_sample_desc.Quality);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
    D3D12_CLEAR_VALUE color_clear_value;
    color_clear_value.Format   = color_tex_desc.Format;
    color_clear_value.Color[0] = 0.0f;
    color_clear_value.Color[1] = 0.0f;
    color_clear_value.Color[2] = 0.0f;
    color_clear_value.Color[3] = 1.0f;
    
    g_msaa_color_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
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
    g_render_target.AttachTexture(AttachmentPoint::Color0, g_msaa_color_texture);
    g_render_target.AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
    
    // View matrix
    v3 eye_pos = { 0, 0,   3 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    g_camera.Init(eye_pos);
    
    // Make sure the device is finished uploading resources
    copy_queue->Flush();
}

void PBR::OnRender(CommandList *command_list, v2 dims)
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
    
    command_list->SetPipelineState(g_pbr_pso._handle);
    command_list->SetGraphicsRootSignature(&g_pbr_signature);
    
    g_light_prop.NumPointLights = (u32)arrlen(g_point_lights);
    g_light_prop.CameraPos = g_camera._position;
    
    command_list->SetGraphics32BitConstants(PBR_RP::LightPropCB, &g_light_prop);
    
    command_list->SetGraphicsDynamicStructuredBuffer(PBR_RP::PointLightSB, arrlen(g_point_lights), 
                                                     sizeof(g_point_lights[0]), g_point_lights);
    
    // Bind textures
    command_list->SetShaderResourceView(PBR_RP::Textures, 0, g_albedo,    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->SetShaderResourceView(PBR_RP::Textures, 1, g_normal,    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->SetShaderResourceView(PBR_RP::Textures, 2, g_metallic,  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->SetShaderResourceView(PBR_RP::Textures, 3, g_roughness, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->SetShaderResourceView(PBR_RP::Textures, 4, g_ao,        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    // Draw scene geometry!
    
    u32 mesh_count = g_nr_rows * g_nr_cols;
    m4 proj_view = m4_mul(projection_matrix, view_matrix);
    for (u32 i = 0; i < mesh_count; ++i)
    {
        command_list->SetGraphicsDynamicConstantBuffer(PBR_RP::MaterialCB, &g_materials[i]);
        
        Mat_CB matrix = {};
        matrix.Model = g_models[i];
        matrix.ModelView = m4_mul(view_matrix, matrix.Model);
        //matrix.InverseTransposeModel = m4_transpose(m4_inverse(matrix.Model));
        matrix.InverseTransposeModel = matrix.Model;
        matrix.ModelViewProjection = m4_mul(proj_view, matrix.Model);
        command_list->SetGraphicsDynamicConstantBuffer(PBR_RP::MatrixCB, &matrix);
        
        RenderSphere(command_list, &g_spheres[i]);
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

void PBR::OnFree()
{
    g_render_target.Free();
    texture::Free(g_final_color_texture);
    texture::Free(g_msaa_color_texture);
    texture::Free(g_depth_texture);
    g_is_active = false;
}

ViewportCamera* PBR::GetViewportCamera()
{
    return &g_camera;
}

void PBR::OnDrawSceneData()
{
    static v4 albedo_color = { 0.5f, 0.0f, 0.0f, 1.0f };
    if (ImGui::ColorEdit4("Albedo Color", albedo_color.p))
    {
        for (u32 i = 0; i < (u32)arrlen(g_materials); ++i)
        {
            g_materials[i].albedo = albedo_color;
        }
    }
}

void PBR::OnDrawSelectedObject()
{
}
