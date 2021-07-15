
// The heightmap downsampler downsamples a high resolution
// heightmap into a low-resolution image. It does this by 
// binding low resolution texture to a rendertarget. The 
// rendertarget writes the 1 channel heightmap image into 
// a low-res 4 channel texture.

struct HeightmapDownsampler
{
    void Init(u32 downsample_width, u32 downsample_height);
    void Free();
    
    // @param src: high resolution texture
    // @param dst: low resolution texture. If the texture does
    //             not match the dimenions of the render target
    //             it is resized to fit it. This texture is bound
    //             to the render target and must have the flag
    //             "D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET"
    void Downsample(CommandList *command_list, TEXTURE_ID src, TEXTURE_ID dst);
    
    enum RootParms
    {
        Param_Heightmap,
        Param_Count,
    };
    
    static constexpr wchar_t *g_vertex_shader = L"shaders/HeightmapDownsample_Vtx.cso";
    static constexpr wchar_t *g_pixel_shader  = L"shaders/HeightmapDownsample_Pxl.cso";
    
    // Shouldn't need a vertex/index buffer because
    // a quad vertex + TEXCOORDS can be calculated
    // within the shader.
    
    RenderTarget        _rt;
    RootSignature       _root_signature;
    PipelineStateObject _pso;
};

void 
HeightmapDownsampler::Init(u32 downsample_width, u32 downsample_height)
{
    D3D12_DESCRIPTOR_RANGE1 texture_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    D3D12_ROOT_PARAMETER1 height_param = d3d::root_param1::InitAsDescriptorTable(1, &texture_range, 
                                                                                 D3D12_SHADER_VISIBILITY_PIXEL);
    
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    D3D12_ROOT_PARAMETER1 root_params[Param_Count];
    root_params[Param_Heightmap] = height_param;
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(g_vertex_shader);
    shader_modules.pixel  = LoadShaderModule(g_pixel_shader);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &_root_signature;
    pso_desc.shader_modules      = shader_modules;
    pso_desc.blend               = GetBlendState(BlendState::Disabled);
    pso_desc.depth               = GetDepthStencilState(DepthStencilState::Disabled);
    pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
    pso_desc.input_layouts       = NULL;
    pso_desc.input_layouts_count = 0;
    pso_desc.topology            = GfxTopology::Triangle;
    pso_desc.sample_desc.count   = 1;
    
    pso_desc.rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.rtv_formats.NumRenderTargets = 1;
    
    _pso.Init(&pso_desc);
    
    downsample_width  = fast_max(1, downsample_width);
    downsample_height = fast_max(1, downsample_height);
    _rt.Init();
    _rt.Resize(downsample_width, downsample_height);
}

void
HeightmapDownsampler::Free()
{
    _rt.Free();
    _root_signature.Free();
    _pso.Free();
}

void 
HeightmapDownsampler::Downsample(CommandList *command_list, TEXTURE_ID src, TEXTURE_ID dst)
{
    // NOTE(Dustin): Do I need to set viewport/scissor?
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    _rt.AttachTexture(AttachmentPoint::Color0, dst);
    command_list->ClearTexture(_rt.GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->SetRenderTarget(&_rt);
    
    D3D12_VIEWPORT viewport = _rt.GetViewport();
    command_list->SetViewport(viewport);
    
    D3D12_RECT scissor_rect = {};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)viewport.Width;
    scissor_rect.bottom = (LONG)viewport.Height;
    command_list->SetScissorRect(scissor_rect);
    
    command_list->SetPipelineState(_pso._handle);
    command_list->SetGraphicsRootSignature(&_root_signature);
    command_list->SetShaderResourceView(Param_Heightmap, 0, src);
    command_list->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    command_list->DrawInstanced(4);
}

