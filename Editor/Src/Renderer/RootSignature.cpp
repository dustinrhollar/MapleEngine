
RenderError
RootSignature::Init(UINT                             anumParameters,
                    const D3D12_ROOT_PARAMETER1     *apParameters,
                    UINT                             anumStaticSamplers,
                    const D3D12_STATIC_SAMPLER_DESC *apStaticSamplers,
                    D3D12_ROOT_SIGNATURE_FLAGS       aflags)
{
    RenderError result = RenderError::Success;
    ID3D12Device *d3d_device = device::GetDevice();
    
    memset(_num_descriptors_per_table, 0, sizeof(u32) * 32);
    _sampler_table_bitmask = 0;
    _descriptor_table_bitmask = 0;
    
    // Create the root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureData = {};
    FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    
    if (FAILED(d3d_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &FeatureData, sizeof(FeatureData))))
    {
        FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }
    
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
    root_sig_desc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_sig_desc.Desc_1_1.Flags             = aflags;
    root_sig_desc.Desc_1_1.NumParameters     = anumParameters;
    root_sig_desc.Desc_1_1.pParameters       = apParameters;
    root_sig_desc.Desc_1_1.NumStaticSamplers = anumStaticSamplers;
    root_sig_desc.Desc_1_1.pStaticSamplers   = apStaticSamplers;
    
    D3D12_ROOT_SIGNATURE_DESC1 *root_sig_desc1 = &root_sig_desc.Desc_1_1;
    
    UINT num_params = root_sig_desc.Desc_1_1.NumParameters;
    D3D12_ROOT_PARAMETER1* pParameters = num_params > 0 ? (D3D12_ROOT_PARAMETER1*)SysAlloc(sizeof(D3D12_ROOT_PARAMETER1) * num_params) : nullptr;
    
    for (UINT i = 0; i < num_params; ++i)
    {
        const D3D12_ROOT_PARAMETER1& rootParameter = root_sig_desc1->pParameters[i];
        pParameters[i] = rootParameter;
        
        if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            UINT numDescriptorRanges = rootParameter.DescriptorTable.NumDescriptorRanges;
            D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges = numDescriptorRanges > 0 ? 
            (D3D12_DESCRIPTOR_RANGE1*)SysAlloc(sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges) : nullptr;
            
            memcpy(pDescriptorRanges, rootParameter.DescriptorTable.pDescriptorRanges,
                   sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);
            
            pParameters[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
            pParameters[i].DescriptorTable.pDescriptorRanges = pDescriptorRanges;
            
            // Set the bit mask depending on the type of descriptor table.
            if (numDescriptorRanges > 0)
            {
                switch (pDescriptorRanges[0].RangeType)
                {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:     _descriptor_table_bitmask |= (1 << i); break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: _sampler_table_bitmask |= (1 << i);    break;
                }
            }
            
            // Count the number of descriptors in the descriptor table.
            for (UINT j = 0; j < numDescriptorRanges; ++j)
            {
                _num_descriptors_per_table[i] += pDescriptorRanges[j].NumDescriptors;
            }
        }
    }
    
    _desc.NumParameters = num_params;
    _desc.pParameters = pParameters;
    
    UINT numStaticSamplers = root_sig_desc1->NumStaticSamplers;
    D3D12_STATIC_SAMPLER_DESC* pStaticSamplers = numStaticSamplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers] : nullptr;
    
    if (pStaticSamplers)
    {
        memcpy(pStaticSamplers, root_sig_desc1->pStaticSamplers, sizeof( D3D12_STATIC_SAMPLER_DESC ) * numStaticSamplers);
    }
    
    _desc.NumStaticSamplers = numStaticSamplers;
    _desc.pStaticSamplers = pStaticSamplers;
    
    D3D12_ROOT_SIGNATURE_FLAGS flags = root_sig_desc1->Flags;
    _desc.Flags = flags;
    
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc;
    versionRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    versionRootSignatureDesc.Desc_1_1.NumParameters = num_params;
    versionRootSignatureDesc.Desc_1_1.pParameters = pParameters;
    versionRootSignatureDesc.Desc_1_1.NumStaticSamplers = numStaticSamplers;
    versionRootSignatureDesc.Desc_1_1.pStaticSamplers = pStaticSamplers;
    versionRootSignatureDesc.Desc_1_1.Flags = flags;
    
    // Serialize the root signature.
    ID3DBlob* rootSignatureBlob;
    ID3DBlob* errorBlob;
    AssertHr(D3D12SerializeVersionedRootSignature(&versionRootSignatureDesc,
                                                  &rootSignatureBlob, 
                                                  &errorBlob));
    
    // Create the root signature.
    AssertHr(d3d_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
                                             rootSignatureBlob->GetBufferSize(), IIDE(&_handle)));
    
    return result;
}

RenderError
RootSignature::Free()
{
    RenderError result = RenderError::Success;
    if (_handle) D3D_RELEASE(_handle);
    memset(_num_descriptors_per_table, 0, sizeof(u32) * 32);
    _sampler_table_bitmask = 0;
    _descriptor_table_bitmask = 0;
    
    return result;
}

u32 RootSignature::GetDescriptorTableBitmask(D3D12_DESCRIPTOR_HEAP_TYPE heap_type)
{
    u32 result = 0;
    switch (heap_type)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: result = _descriptor_table_bitmask; break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:     result = _sampler_table_bitmask;    break;
    }
    return result;
}

u32 RootSignature::GetNumDescriptors(u32 root_idx)
{
    assert(root_idx < 32);
    return _num_descriptors_per_table[root_idx];
}