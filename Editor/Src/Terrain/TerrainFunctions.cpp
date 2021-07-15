
namespace terrain
{
    enum ComputeFunction
    {
        Function_Checker,
        Function_Discrete,
        Function_LinearValue,
        Function_FadedValue,
        Function_CubicValue,
        Function_Perlin,
        Function_Simplex,
        Function_Worley,
        Function_Spots,
        
        Function_Count,
    };
    
    enum ComputeParameters
    {
        NoiseCB_CS,
        HeightmapTexture_CS,
        
        NumParams_CS,
    };
    
    struct Noise_CB
    {
        r32 scale;
        i32 seed;
        i32 octaves;
        r32 lacunarity;
        r32 decay;
        r32 threshold; // for bounded noise
    };
    
    void LoadComputeFunctions();
    void ReleaseComputeFunctions();
    
    void ExecuteFunction(ComputeFunction fn, Noise_CB *cb, CommandList *command_list, TEXTURE_ID texture);
    
    RootSignature       g_compute_signature;
    PipelineStateObject g_compute_pso[Function_Count];
    
    static FORCE_INLINE ID3D12PipelineState*
        CreatePso(RootSignature *root_signature, ID3DBlob *blob)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC compute_desc = {};
        compute_desc.pRootSignature = root_signature->_handle;
        compute_desc.CS = { (UINT8*)(blob->GetBufferPointer()), blob->GetBufferSize() };
        compute_desc.NodeMask = 0;
        compute_desc.CachedPSO = {};
        compute_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        
        ID3D12PipelineState *resource = 0;
        AssertHr(device::GetDevice()->CreateComputePipelineState(&compute_desc, IIDE(&resource)));
        return resource;
    }
    
}; // terrain


void terrain::LoadComputeFunctions()
{
    D3D12_ROOT_PARAMETER1 data_param = d3d::root_param1::InitAsConstant(sizeof(Noise_CB) / 4, 0, 0,
                                                                        D3D12_SHADER_VISIBILITY_ALL);
    D3D12_DESCRIPTOR_RANGE1 heightmap_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
                                                                       D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    
    D3D12_ROOT_PARAMETER1 root_parameters[ComputeParameters::NumParams_CS];
    root_parameters[ComputeParameters::NoiseCB_CS] = data_param;
    root_parameters[ComputeParameters::HeightmapTexture_CS] = d3d::root_param1::InitAsDescriptorTable(1, &heightmap_range);
    
    D3D12_STATIC_SAMPLER_DESC linear_wrap_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                                                              D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                                              D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    
    g_compute_signature = {};
    g_compute_signature.Init(ComputeParameters::NumParams_CS, root_parameters, 1, &linear_wrap_sampler);
    
    // For now, just load the Perlin Compute Kernel
    {
        ID3DBlob *cs = (ID3DBlob*)LoadShaderModule(L"shaders/PerlinNoise.cso");
        g_compute_pso[Function_Perlin]._handle = CreatePso(&g_compute_signature, cs);
    }
    
}

void terrain::ReleaseComputeFunctions()
{
    g_compute_signature.Free();
    g_compute_pso[Function_Perlin].Free();
}

void terrain::ExecuteFunction(ComputeFunction fn, Noise_CB *cb, CommandList *command_list, TEXTURE_ID texture)
{
    assert(fn == Function_Perlin && "Only Perlin Noise is supported right now!");
    
    D3D12_RESOURCE_DESC desc = texture::GetResourceDesc(texture);
    
    // TODO(Dustin): Constant Buffer parameters 
    
    command_list->SetComputeRootSignature(&g_compute_signature);
    command_list->SetPipelineState(g_compute_pso[fn]._handle);
    command_list->SetCompute32BitConstants(NoiseCB_CS, *cb);
    command_list->SetUnorderedAccessView(ComputeParameters::HeightmapTexture_CS, 0, texture, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                         0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    
    
    command_list->Dispatch(divide_align((u32)desc.Width, 8), divide_align(desc.Height, 8));
}

