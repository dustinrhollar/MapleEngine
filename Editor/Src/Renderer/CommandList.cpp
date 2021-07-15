
void CommandList::GenerateMipsPSO::Init()
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    D3D12_DESCRIPTOR_RANGE1 srcMip = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                                                              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    D3D12_DESCRIPTOR_RANGE1 outMip = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0,
                                                              D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    
    D3D12_ROOT_PARAMETER1 rootParameters[GenerateMips::NumRootParameters];
    rootParameters[(u32)GenerateMips::GenerateMipsCB] = d3d::root_param1::InitAsConstant(sizeof(GenerateMipsCB) / 4, 0);
    rootParameters[(u32)GenerateMips::SrcMip] = d3d::root_param1::InitAsDescriptorTable(1, &srcMip);
    rootParameters[(u32)GenerateMips::OutMip] = d3d::root_param1::InitAsDescriptorTable(1, &outMip);
    
    D3D12_STATIC_SAMPLER_DESC linearClampSampler = d3d::GetStaticSamplerDesc( 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                                                             D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                             D3D12_TEXTURE_ADDRESS_MODE_CLAMP );
    
    _root_signature = {};
    _root_signature.Init((u32)GenerateMips::NumRootParameters, rootParameters, 1, &linearClampSampler);
    
    ID3DBlob *cs = (ID3DBlob*)LoadShaderModule(L"shaders/GenerateMips_CS.cso");
    
    // Create the PSO for GenerateMips shader.
    D3D12_COMPUTE_PIPELINE_STATE_DESC compute_desc = {};
    compute_desc.pRootSignature = _root_signature._handle;
    compute_desc.CS = { (UINT8*)(cs->GetBufferPointer()), cs->GetBufferSize() };
    compute_desc.NodeMask = 0;
    compute_desc.CachedPSO = {};
    compute_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    
    _pipeline_state = {};
    AssertHr(d3d_device->CreateComputePipelineState(&compute_desc, IIDE(&_pipeline_state._handle)));
    
    // Create some default texture UAV's to pad any unused UAV's during mip map generation.
    _default_uav = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);
    
    for ( UINT i = 0; i < 4; ++i )
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Format                           = DXGI_FORMAT_R8G8B8A8_UNORM;
        uavDesc.Texture2D.MipSlice               = i;
        uavDesc.Texture2D.PlaneSlice             = 0;
        
        d3d_device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, _default_uav.GetDescriptorHandle(i));
    }
}

void CommandList::GenerateMipsPSO::Free()
{
    _root_signature.Free();
    _pipeline_state.Free();
}

RenderError 
CommandList::Init(D3D12_COMMAND_LIST_TYPE list_type)
{
    RenderError result = RenderError::Success;
    ID3D12Device *d3d_device = device::GetDevice();
    
    _type = list_type;
    
    AssertHr(d3d_device->CreateCommandAllocator(_type, IIDE(&_allocator)));
    AssertHr(d3d_device->CreateCommandList(0, _type, _allocator, nullptr, IIDE(&_handle)));
    
    _upload_buffer.Init();
    _resource_state_tracker.Init();
    
    for (i32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        _dynamic_descriptor_heap[i].Init((D3D12_DESCRIPTOR_HEAP_TYPE)i);
        _descriptor_heaps[i] = 0;
    }
    
    _generate_mips_pso.Init();
    _compute_command_list = 0;
    
    return result;
}

RenderError 
CommandList::Free()
{
    RenderError result = RenderError::Success;
    
    _generate_mips_pso.Free();
    
    D3D_RELEASE(_allocator);
    D3D_RELEASE(_handle);
    
    _upload_buffer.Free();
    _resource_state_tracker.Free();
    
    for (i32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        _dynamic_descriptor_heap[i].Free();
        _descriptor_heaps[i] = 0;
    }
    
    return result;
}

void 
CommandList::Reset()
{
    AssertHr(_allocator->Reset());
    AssertHr(_handle->Reset(_allocator, nullptr));
    
    _resource_state_tracker.Reset();
    _upload_buffer.Reset();
    
    ReleaseTrackedObjects();
    
    for (u32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        _dynamic_descriptor_heap[i].Reset();
        _descriptor_heaps[i] = nullptr;
    }
    
    _root_signature = 0;
    _pipeline_state = 0;
    _compute_command_list = 0;
}

void 
CommandList::Close()
{
    FlushResourceBarriers();
    _handle->Close();
}

void 
CommandList::FlushResourceBarriers()
{
    _resource_state_tracker.FlushResourceBarriers(this);
}

bool 
CommandList::ClosePending(CommandList *pending_cmd_list)
{
    // Flush any remaining barriers.
    FlushResourceBarriers();
    
    _handle->Close();
    
    // Flush pending resource barriers.
    u32 num_pending = _resource_state_tracker.FlushPendingResourceBarriers(pending_cmd_list);
    // Commit the final resource state to the global state.
    _resource_state_tracker.CommitFinalResourceStates();
    
    return num_pending > 0;
}

void 
CommandList::SetGraphics32BitConstants(u32 rootParameterIndex, u32 numConstants, void* constants)
{
    _handle->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void 
CommandList::ClearTexture(TEXTURE_ID tex_id, r32 clear_color[4])
{
    Texture *texture = texture::GetTexture(tex_id);
    assert(texture);
    
    TransitionBarrier(texture->_resource._handle, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    _handle->ClearRenderTargetView(texture->GetRenderTargetView(), clear_color, 0, nullptr);
    
    //TrackResource(texture->_resource._handle);
}

void CommandList::ClearDepthStencilTexture(TEXTURE_ID texture_id, 
                                           D3D12_CLEAR_FLAGS clear_flags,
                                           r32 depth,
                                           u8 stencil)
{
    assert(texture::IsValid(texture_id));
    
    Texture *texture = texture::GetTexture(texture_id);
    TransitionBarrier(texture->_resource._handle, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    _handle->ClearDepthStencilView(texture->GetDepthStencilView(), clear_flags, depth, stencil, 0, nullptr);
    
    //TrackResource(texture->_resource._handle);
}

void 
CommandList::SetRenderTarget(RenderTarget *render_target)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptors[AttachmentPoint::NumAttachmentPoints];
    u32 descriptor_count = 0;
    
    TEXTURE_ID *texture_ids = render_target->_textures;
    
    for ( int i = 0; i < 8; ++i )
    {
        Texture *texture = texture::GetTexture(texture_ids[i]);
        if (texture)
        {
            TransitionBarrier(texture->_resource._handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
            rt_descriptors[descriptor_count++] = texture->GetRenderTargetView();
            TrackResource(texture->_resource._handle);
        }
    }
    
    TEXTURE_ID depth_texture_id = render_target->GetTexture(AttachmentPoint::DepthStencil);
    D3D12_CPU_DESCRIPTOR_HANDLE depth_handle = {};
    
    Texture *depth_texture = texture::GetTexture(depth_texture_id);
    if (depth_texture)
    {
        TransitionBarrier(depth_texture->_resource._handle, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        depth_handle = depth_texture->GetDepthStencilView();
        TrackResource(depth_texture->_resource._handle);
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE* dsv = depth_handle.ptr != 0 ? &depth_handle : nullptr;
    
    _handle->OMSetRenderTargets(descriptor_count, rt_descriptors, FALSE, dsv);
}

void
CommandList::TransitionBarrier(ID3D12Resource*       resource,
                               D3D12_RESOURCE_STATES after,
                               UINT                  subresource,
                               bool                  flush_barriers)
{
    if (resource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = after;
        barrier.Transition.Subresource = subresource;
        _resource_state_tracker.ResourceBarrier(&barrier);
    }
    
    if (flush_barriers)
    {
        FlushResourceBarriers();
    }
}

void 
CommandList::UAVBarrier(ID3D12Resource* resource, bool flush_barriers)
{
    D3D12_RESOURCE_BARRIER barrier = d3d::GetUAVBarrier(resource);
    
    _resource_state_tracker.ResourceBarrier(&barrier);
    if (flush_barriers)
    {
        FlushResourceBarriers();
    }
}

void 
CommandList::AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flush_barriers)
{
    D3D12_RESOURCE_BARRIER barrier = d3d::GetAliasingBarrier(beforeResource, afterResource);
    
    _resource_state_tracker.ResourceBarrier(&barrier);
    if (flush_barriers)
    {
        FlushResourceBarriers();
    }
}

void 
CommandList::CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes)
{
    assert(dstRes);
    assert(srcRes);
    
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);
    
    FlushResourceBarriers();
    
    _handle->CopyResource(dstRes, srcRes);
    
    TrackResource(dstRes);
    TrackResource(srcRes);
}

void 
CommandList::CopyResource(Resource* dstRes, Resource* srcRes)
{
    CopyResource(dstRes->_handle, srcRes->_handle);
}

void 
CommandList::CopyTextureSubresource(TEXTURE_ID tex_id, u32 first_subresource, u32 num_subresources,
                                    D3D12_SUBRESOURCE_DATA *subresources)
{
    assert(texture::IsValid(tex_id));
    
    ID3D12Device *d3d_device = device::GetDevice();
    ID3D12Resource *dst_rsrc = texture::GetResource(tex_id)->_handle;
    
    if (dst_rsrc)
    {
        TransitionBarrier(dst_rsrc, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        
        UINT64 req_sz = d3d::GetRequiredIntermediateSize(dst_rsrc, first_subresource, num_subresources);
        
        ID3D12Resource *iterim_rsrc;
        AssertHr(d3d_device->CreateCommittedResource(&d3d::GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD), 
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &d3d::GetBufferResourceDesc(req_sz), 
                                                     D3D12_RESOURCE_STATE_GENERIC_READ, 
                                                     nullptr,
                                                     IIDE(&iterim_rsrc)));
        
        d3d::UpdateSubresources(_handle, dst_rsrc, iterim_rsrc, 0, first_subresource, num_subresources, subresources);
        
        TrackResource(iterim_rsrc);
        TrackResource(dst_rsrc);
    }
}

TEXTURE_ID 
CommandList::LoadTextureFromFile(const char *filename, bool gen_mipmaps)
{
    // TODO(Dustin):
    //- Is UINT Format what I want?
    //- Detect sRGB 
    //- Detect multiple subresources
    //- Generate mipmaps
    
    const int desired_channels = 4;
    int x, y, channels;
    unsigned char *data = stbi_load(filename, &x, &y, &channels, desired_channels);
    
    TEXTURE_ID result = LoadTextureFromMemory(data, x, y, desired_channels, gen_mipmaps );
    // NOTE(Dustin): This free might have to wait until the command list is done executing...
    stbi_image_free(data);
    return result;
}

TEXTURE_ID 
CommandList::LoadTextureFromMemory(void *pixels, int width, int height, int num_channels, bool gen_mips)
{
    D3D12_RESOURCE_DESC rsrc_desc = d3d::GetTex2DDesc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    
    TEXTURE_ID result = texture::Create(&rsrc_desc);
    assert(texture::IsValid(result));
    ID3D12Resource *resource = texture::GetResource(result)->_handle;
    
    ResourceStateTracker::AddGlobalResourceState(resource, D3D12_RESOURCE_STATE_COMMON);
    
    D3D12_SUBRESOURCE_DATA subresource = {};
    subresource.RowPitch = width * (sizeof(u8) * 4);
    subresource.SlicePitch = subresource.RowPitch * height;
    subresource.pData = pixels;
    
    CopyTextureSubresource(result, 0, 1, &subresource);
    
    if (gen_mips)
    {
        GenerateMips(result);
    }
    
    return result;
}

void 
CommandList::GenerateMips(TEXTURE_ID tex_id)
{
    assert(texture::IsValid(tex_id));
    ID3D12Device *d3d_device = device::GetDevice();
    
    if (_type == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        if (!_compute_command_list)
        {
            _compute_command_list = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList();
        }
        _compute_command_list->GenerateMips(tex_id);
        return;
    }
    
    Texture *texture = texture::GetTexture(tex_id);
    assert(texture);
    
    Resource *rsrc = &texture->_resource;
    D3D12_RESOURCE_DESC desc = rsrc->GetResourceDesc();
    ID3D12Resource *d3d12_rsrc = rsrc->_handle;
    
    // if the resource only has a signle mip level, do nothing
    // NOTE(Dustin): Need to watch this branch when testing
    if (desc.MipLevels == 1)
    {
        return;
    }
    
    // Currently, only non-multi-sampled 2D textures are supported
    assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    assert(desc.DepthOrArraySize == 1);
    assert(desc.SampleDesc.Count == 1);
    
    ID3D12Resource *uav_rsrc = d3d12_rsrc;
    // This is done to perform a GPU copy of resources with different formats.
    // BGR -> RGB texture copies will fail GPU validation unless performed
    // through an alias of the BRG resource in a placed heap.
    ID3D12Resource *alias_rsrc;
    
    // If the passed-in resource does not allow for UAV access
    // then create a staging resource that is used to generate
    // the mipmap chain.
    if (!texture->CheckUAVSupport() || (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
    {
        // Describe an alias resource that is used to copy the original texture.
        D3D12_RESOURCE_DESC aliasDesc = desc;
        // Placed resources can't be render targets or depth-stencil views.
        aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        aliasDesc.Flags &= ~( D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL );
        
        // Describe a UAV compatible resource that is used to perform
        // mipmapping of the original texture.
        D3D12_RESOURCE_DESC uavDesc = aliasDesc;  // The flags for the UAV description must match that of the alias description.
        uavDesc.Format = Texture::GetUAVCompatableFormat(desc.Format);
        
        D3D12_RESOURCE_DESC resourceDescs[] = { aliasDesc, uavDesc };
        
        // Create a heap that is large enough to store a copy of the original resource.
        D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = d3d_device->GetResourceAllocationInfo(0, _countof( resourceDescs ), resourceDescs);
        
        D3D12_HEAP_DESC heapDesc                 = {};
        heapDesc.SizeInBytes                     = allocationInfo.SizeInBytes;
        heapDesc.Alignment                       = allocationInfo.Alignment;
        heapDesc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        heapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        
        ID3D12Heap *heap;
        AssertHr(d3d_device->CreateHeap(&heapDesc, IIDE(&heap)));
        
        // Make sure the heap does not go out of scope until the command list
        // is finished executing on the command queue.
        TrackObject(heap);
        
        // Create a placed resource that matches the description of the
        // original resource. This resource is used to copy the original
        // texture to the UAV compatible resource.
        AssertHr(d3d_device->CreatePlacedResource(heap, 0, &aliasDesc, D3D12_RESOURCE_STATE_COMMON,
                                                  nullptr, IIDE(&alias_rsrc)));
        
        ResourceStateTracker::AddGlobalResourceState(alias_rsrc, D3D12_RESOURCE_STATE_COMMON);
        // Ensure the scope of the alias resource.
        TrackResource(alias_rsrc);
        
        // Create a UAV compatible resource in the same heap as the alias
        // resource.
        AssertHr(d3d_device->CreatePlacedResource(heap, 0, &uavDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                  IIDE(&uav_rsrc)));
        
        ResourceStateTracker::AddGlobalResourceState(uav_rsrc, D3D12_RESOURCE_STATE_COMMON);
        
        // Ensure the scope of the UAV compatible resource.
        TrackResource(uav_rsrc);
        
        // Add an aliasing barrier for the alias resource.
        AliasingBarrier(nullptr, alias_rsrc);
        
        // Copy the original resource to the alias resource.
        // This ensures GPU validation.
        CopyResource(alias_rsrc, d3d12_rsrc);
        
        // Add an aliasing barrier for the UAV compatible resource.
        AliasingBarrier(alias_rsrc, uav_rsrc);
    }
    
    // Generate mips with the UAV compatible resource.
    TEXTURE_ID uavTexture = texture::Create(uav_rsrc);
    assert(texture::IsValid(uavTexture));
    GenerateMips_UAV(uavTexture, Texture::IsSRGBFormat(desc.Format));
    
    if (alias_rsrc)
    {
        AliasingBarrier(uav_rsrc, alias_rsrc);
        // Copy the alias resource back to the original resource.
        CopyResource(d3d12_rsrc, alias_rsrc);
    }
    
    // TODO(Dustin): How/When to free the UAV texture?
}

void 
CommandList::GenerateMips_UAV(TEXTURE_ID tex_id, bool isSRGB)
{
    SetPipelineState(_generate_mips_pso._pipeline_state._handle);
    SetComputeRootSignature(&_generate_mips_pso._root_signature);
    
    GenerateMipsCB generateMipsCB = {};
    generateMipsCB.IsSRGB = isSRGB;
    
    Resource *resource = texture::GetResource(tex_id);
    D3D12_RESOURCE_DESC resourceDesc = texture::GetResourceDesc(tex_id);
    
    // Create an SRV that uses the format of the original texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                  = isSRGB ? Texture::GetSRGBFormat(resourceDesc.Format) : resourceDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension =
        D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
    srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
    
    ShaderResourceView srv = {};
    srv.Init(resource, &srvDesc);
    
    for (u32 srcMip = 0; srcMip < resourceDesc.MipLevels - 1u;)
    {
        u64 srcWidth  = resourceDesc.Width >> srcMip;
        u32 srcHeight = resourceDesc.Height >> srcMip;
        u32 dstWidth  = static_cast<u32>( srcWidth >> 1 );
        u32 dstHeight = srcHeight >> 1;
        
        // 0b00(0): Both width and height are even.
        // 0b01(1): Width is odd, height is even.
        // 0b10(2): Width is even, height is odd.
        // 0b11(3): Both width and height are odd.
        generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);
        
        // How many mipmap levels to compute this pass (max 4 mips per pass)
        DWORD mipCount;
        
        // The number of times we can half the size of the texture and get
        // exactly a 50% reduction in size.
        // A 1 bit in the width or height indicates an odd dimension.
        // The case where either the width or the height is exactly 1 is handled
        // as a special case (as the dimension does not require reduction).
        _BitScanForward(&mipCount,
                        (dstWidth == 1 ? dstHeight : dstWidth) | (dstHeight == 1 ? dstWidth : dstHeight));
        // Maximum number of mips to generate is 4.
        mipCount = fast_min(4, mipCount + 1);
        // Clamp to total number of mips left over.
        mipCount = (srcMip + mipCount) >= resourceDesc.MipLevels ? resourceDesc.MipLevels - srcMip - 1 : mipCount;
        
        // Dimensions should not reduce to 0.
        // This can happen if the width and height are not the same.
        dstWidth  = fast_max(1, dstWidth);
        dstHeight = fast_max(1, dstHeight);
        
        generateMipsCB.SrcMipLevel  = srcMip;
        generateMipsCB.NumMipLevels = mipCount;
        generateMipsCB.TexelSize.x  = 1.0f / (float)dstWidth;
        generateMipsCB.TexelSize.y  = 1.0f / (float)dstHeight;
        
        SetCompute32BitConstants((u32)GenerateMips::GenerateMipsCB, generateMipsCB);
        
        SetShaderResourceView((u32)GenerateMips::SrcMip, 0, &srv, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1);
        
        for (u32 mip = 0; mip < mipCount; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format                           = resourceDesc.Format;
            uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice               = srcMip + mip + 1;
            
            UnorderedAccessView uav = {};
            uav.Init(resource, nullptr, &uavDesc);
            SetUnorderedAccessView((u32)GenerateMips::OutMip, mip, &uav, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                   srcMip + mip + 1, 1);
        }
        
        // Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
        if (mipCount < 4)
        {
            _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors((u32)GenerateMips::OutMip, mipCount, 4 - mipCount, _generate_mips_pso.GetDefaultUAV() );
        }
        
        SetComputeRootSignature(&_generate_mips_pso._root_signature);
        Dispatch(divide_align(dstWidth, 8), divide_align(dstHeight, 8));
        
        UAVBarrier(resource->_handle);
        
        srcMip += mipCount;
    }
    
    // TODO(Dustin): Clean up the SRV + descriptors?
}

// Set an inline CBV.
// Note: Only Buffer resoruces can be used with inline UAV's.
void 
CommandList::SetConstantBufferView(u32 root_param_idx, Resource *buffer,
                                   D3D12_RESOURCE_STATES stateAfter,
                                   u64 buffer_offset)
{
    if (buffer)
    {
        TransitionBarrier(buffer->_handle, stateAfter);
        _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageInlineCBV(root_param_idx, 
                                                                                        buffer->_handle->GetGPUVirtualAddress() + buffer_offset);
        TrackResource(buffer->_handle);
    }
}

// Set the CBV on the rendering pipeline.
void 
CommandList::SetConstantBufferView(u32 root_param_idx, u32 descriptorOffset,
                                   ConstantBufferView *cbv,
                                   D3D12_RESOURCE_STATES stateAfter)
{
    assert(cbv);
    
    ConstantBuffer *constantBuffer = cbv->_cb;
    if (constantBuffer)
    {
        TransitionBarrier(constantBuffer->_resource._handle, stateAfter);
        TrackResource(constantBuffer->_resource._handle);
    }
    
    _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors(root_param_idx, descriptorOffset, 1, cbv->GetDescriptorHandle());
}

// Set an inline SRV.
// Note: Only Buffer resources can be used with inline SRV's
void 
CommandList::SetShaderResourceView(u32 root_param_idx, Resource *buffer,
                                   D3D12_RESOURCE_STATES after,
                                   u64 buffer_offset)
{
    if (buffer)
    {
        TransitionBarrier(buffer->_handle, after);
        _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageInlineSRV(root_param_idx, 
                                                                                        buffer->_handle->GetGPUVirtualAddress() + buffer_offset);
        TrackResource(buffer->_handle);
    }
}

// Set the SRV on the graphics pipeline.
void 
CommandList::SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset,
                                   ShaderResourceView *srv,
                                   D3D12_RESOURCE_STATES stateAfter,
                                   UINT firstSubresource,
                                   UINT numSubresources)
{
    assert(srv);
    
    Resource *resource = srv->_resource;
    if (resource)
    {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            for (u32 i = 0; i < numSubresources; ++i)
            {
                TransitionBarrier(resource->_handle, stateAfter, firstSubresource + i);
            }
        }
        else
        {
            TransitionBarrier(resource->_handle, stateAfter);
        }
        
        TrackResource(resource->_handle);
    }
    
    _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors(rootParameterIndex, descriptorOffset, 1, srv->GetDescriptorHandle());
}

// Set an SRV on the graphics pipeline using the default SRV for the texture.
void 
CommandList::SetShaderResourceView(i32 rootParameterIndex, u32 descriptorOffset,
                                   TEXTURE_ID texture_id,
                                   D3D12_RESOURCE_STATES stateAfter,
                                   UINT firstSubresource,
                                   UINT numSubresources)
{
    Texture *texture = texture::GetTexture(texture_id);
    Resource *resource = &texture->_resource;
    if (texture)
    {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            for (u32  i = 0; i < numSubresources; ++i)
            {
                TransitionBarrier(resource->_handle, stateAfter, firstSubresource + i);
            }
        }
        else
        {
            TransitionBarrier(resource->_handle, stateAfter);
        }
        
        TrackResource(resource->_handle);
        
        _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors(rootParameterIndex, descriptorOffset, 1, texture->GetShaderResourceView());
    }
}

//Set an inline UAV.
//Note: Only Buffer resoruces can be used with inline UAV's.
void 
CommandList::SetUnorderedAccessView(u32 root_param_idx, Resource *buffer,
                                    D3D12_RESOURCE_STATES stateAfter,
                                    u64 buffer_offset)
{
    if (buffer)
    {
        TransitionBarrier(buffer->_handle, stateAfter);
        _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageInlineUAV(root_param_idx, 
                                                                                        buffer->_handle->GetGPUVirtualAddress() + buffer_offset);
        TrackResource(buffer->_handle);
    }
}

// Set the UAV on the graphics pipeline.
void 
CommandList::SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
                                    UnorderedAccessView *uav,
                                    D3D12_RESOURCE_STATES stateAfter,
                                    UINT                  firstSubresource,
                                    UINT                  numSubresources)
{
    assert(uav);
    
    Resource *resource = uav->_resource;
    if (resource)
    {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            for (u32 i = 0; i < numSubresources; ++i)
            {
                TransitionBarrier(resource->_handle, stateAfter, firstSubresource + i);
            }
        }
        else
        {
            TransitionBarrier(resource->_handle, stateAfter);
        }
        
        TrackResource(resource->_handle);
    }
    
    _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors(rootParameterIndex, descriptorOffset, 1, uav->GetDescriptorHandle() );
}


// Set the UAV on the graphics pipline using a specific mip of the texture.
void 
CommandList::SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
                                    TEXTURE_ID texture_id, UINT mip,
                                    D3D12_RESOURCE_STATES stateAfter,
                                    UINT                  firstSubresource,
                                    UINT                  numSubresources)
{
    Texture *texture = texture::GetTexture(texture_id);
    Resource *resource = &texture->_resource;
    if (texture)
    {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            for (u32  i = 0; i < numSubresources; ++i)
            {
                TransitionBarrier(resource->_handle, stateAfter, firstSubresource + i);
            }
        }
        else
        {
            TransitionBarrier(resource->_handle, stateAfter);
        }
        
        TrackResource(resource->_handle);
        
        _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].StageDescriptors(rootParameterIndex, descriptorOffset, 1, texture->GetUnorderedAccessView(mip));
    }
}

void 
CommandList::ResolveSubresource(Resource* dst_resource, Resource* src_resource, 
                                u32 dstSubresource, u32 srcSubresource)
{
    assert(dst_resource);
    assert(src_resource);
    
    TransitionBarrier(dst_resource->_handle, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource );
    TransitionBarrier(src_resource->_handle, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource );
    
    FlushResourceBarriers();
    
    _handle->ResolveSubresource(dst_resource->_handle, dstSubresource,
                                src_resource->_handle, srcSubresource,
                                dst_resource->GetResourceDesc().Format );
    
    TrackResource(src_resource->_handle);
    TrackResource(dst_resource->_handle);
}

ID3D12Resource* 
CommandList::CopyBuffer(u64 bufferSize, const void* bufferData,
                        D3D12_RESOURCE_FLAGS flags)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    ID3D12Resource* result;
    if ( bufferSize == 0 )
    {
        // This will result in a NULL resource (which may be desired to define a default null resource).
    }
    else
    {
        D3D12_HEAP_PROPERTIES heap_prop = d3d::GetHeapProperties();
        D3D12_RESOURCE_DESC buffer_desc = d3d::GetBufferResourceDesc(bufferSize, flags);
        
        AssertHr(d3d_device->CreateCommittedResource(&heap_prop, 
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &buffer_desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                     IIDE( &result ) ) );
        
        
        // Add the resource to the global resource state tracker.
        ResourceStateTracker::AddGlobalResourceState(result, D3D12_RESOURCE_STATE_COMMON);
        
        if ( bufferData != nullptr )
        {
            // Create an upload resource to use as an intermediate buffer to copy the buffer resource
            ID3D12Resource* uploadResource;
            AssertHr(d3d_device->CreateCommittedResource(&d3d::GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD), 
                                                         D3D12_HEAP_FLAG_NONE,
                                                         &d3d::GetBufferResourceDesc(bufferSize), 
                                                         D3D12_RESOURCE_STATE_GENERIC_READ, 
                                                         nullptr,
                                                         IIDE(&uploadResource)));
            
            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData                  = bufferData;
            subresourceData.RowPitch               = bufferSize;
            subresourceData.SlicePitch             = subresourceData.RowPitch;
            
            _resource_state_tracker.TransitionResource(result, D3D12_RESOURCE_STATE_COPY_DEST );
            FlushResourceBarriers();
            
            d3d::UpdateSubresources(_handle, result, uploadResource, 0, 0, 1, &subresourceData);
            
            // Add references to resources so they stay in scope until the command list is reset.
            TrackResource(uploadResource);
        }
        
        TrackResource(result);
    }
    
    return result;
}

void 
CommandList::CopyVertexBuffer(VertexBuffer* vertexBuffer, u64 numVertices, u64 vertexStride, void* vertexBufferData)
{
    ID3D12Resource *resource = CopyBuffer(numVertices * vertexStride, vertexBufferData);
    vertexBuffer->Init(resource, numVertices, vertexStride);
}

void 
CommandList::CopyIndexBuffer(IndexBuffer* indexBuffer, u64 numIndices, u64 indexStride, void* indexBufferData)
{
    ID3D12Resource *resource = CopyBuffer(numIndices * indexStride, indexBufferData);
    indexBuffer->Init(resource, numIndices, indexStride);
}

void 
CommandList::SetVertexBuffers(u32 startSlot,
                              VertexBuffer** vertexBuffers,
                              u32 buf_count)
{
    D3D12_VERTEX_BUFFER_VIEW *views = 0;
    arrsetcap(views, buf_count);
    
    for (u32 i = 0; i < buf_count; ++i)
    {
        if (vertexBuffers[i])
        {
            TransitionBarrier(vertexBuffers[i]->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            TrackResource(vertexBuffers[i]->GetResource());
            arrput(views, vertexBuffers[i]->_buffer_view);
        }
    }
    
    _handle->IASetVertexBuffers(startSlot, buf_count, views);
}

void 
CommandList::SetVertexBuffer(u32 slot, VertexBuffer* vertex_buffer )
{
    D3D12_VERTEX_BUFFER_VIEW views[1];
    
    if (vertex_buffer)
    {
        TransitionBarrier(vertex_buffer->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        TrackResource(vertex_buffer->GetResource());
        views[0] = vertex_buffer->_buffer_view;
    }
    
    _handle->IASetVertexBuffers(slot, 1, views);
}

void 
CommandList::SetDynamicVertexBuffer(u32 slot, u64 numVertices, u64 vertexSize, void* vertexBufferData)
{
    u64 bufferSize = numVertices * vertexSize;
    
    UploadBuffer::Allocation heapAllocation = _upload_buffer.Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.cpu, vertexBufferData, bufferSize);
    
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation           = heapAllocation.gpu;
    vertexBufferView.SizeInBytes              = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes            = static_cast<UINT>(vertexSize);
    
    _handle->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void CommandList::SetIndexBuffer(IndexBuffer* indexBuffer)
{
    if (indexBuffer)
    {
        TransitionBarrier(indexBuffer->GetResource(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
        TrackResource(indexBuffer->GetResource());
        _handle->IASetIndexBuffer(&indexBuffer->_buffer_view);
    }
}

void CommandList::SetDynamicIndexBuffer(u64 numIndicies, DXGI_FORMAT indexFormat, void* indexBufferData)
{
    u64 indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    u64 bufferSize       = numIndicies * indexSizeInBytes;
    
    auto heapAllocation = _upload_buffer.Allocate( bufferSize, indexSizeInBytes );
    memcpy(heapAllocation.cpu, indexBufferData, bufferSize);
    
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation          = heapAllocation.gpu;
    indexBufferView.SizeInBytes             = static_cast<UINT>(bufferSize);
    indexBufferView.Format                  = indexFormat;
    
    _handle->IASetIndexBuffer(&indexBufferView);
}


void CommandList::SetCompute32BitConstants( u32 rootParameterIndex, u32 numConstants, const void* constants )
{
    _handle->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetComputeRootSignature(RootSignature *root_sig)
{
    assert(root_sig);
    
    ID3D12RootSignature* root_handle = root_sig->_handle;
    if (_root_signature != root_handle )
    {
        _root_signature = root_handle;
        
        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            _dynamic_descriptor_heap[i].ParseRootSignature(root_sig);
        }
        
        _handle->SetComputeRootSignature(_root_signature);
        TrackObject(_root_signature);
    }
}

void CommandList::SetGraphicsRootSignature(RootSignature *root_sig)
{
    assert(root_sig);
    
    ID3D12RootSignature* root_handle = root_sig->_handle;
    if (_root_signature != root_handle )
    {
        _root_signature = root_handle;
        
        for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
        {
            _dynamic_descriptor_heap[i].ParseRootSignature(root_sig);
        }
        
        _handle->SetGraphicsRootSignature(_root_signature);
        TrackObject(_root_signature);
    }
}

void 
CommandList::SetScissorRect(const D3D12_RECT &scissor_rect)
{
    SetScissorRects(&scissor_rect, 1);
}

void 
CommandList::SetScissorRects(const D3D12_RECT *scissor_rects, u32 num_rects)
{
    assert(num_rects < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    _handle->RSSetScissorRects((UINT)num_rects, scissor_rects);
}

void 
CommandList::SetViewport(const D3D12_VIEWPORT &viewport)
{
    SetViewports(&viewport, 1);
}

void 
CommandList::SetViewports(const D3D12_VIEWPORT *viewports, u32 num_viewports)
{
    assert(num_viewports < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    _handle->RSSetViewports((UINT)num_viewports, viewports);
}

void 
CommandList::SetPipelineState(ID3D12PipelineState *pipeline_state)
{
    if (_pipeline_state != pipeline_state)
    {
        _pipeline_state = pipeline_state;
        _handle->SetPipelineState(pipeline_state);
        TrackObject(pipeline_state);
    }
}

void 
CommandList::SetTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    _handle->IASetPrimitiveTopology(topology);
}

/**
     * Dispatch a compute shader.
     */
void 
CommandList::Dispatch(u32 numGroupsX, u32 numGroupsY, u32 numGroupsZ)
{
    FlushResourceBarriers();
    
    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        _dynamic_descriptor_heap[i].CommitStagedDescriptorsForDispatch(this);
    }
    
    _handle->Dispatch( numGroupsX, numGroupsY, numGroupsZ );
}

void 
CommandList::DrawInstanced(u32 vtx_count_per_instance,
                           u32 instance_count,
                           u32 start_vtx_location,
                           u32 start_instance_location)
{
    FlushResourceBarriers();
    
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        _dynamic_descriptor_heap[i].CommitStagedDescriptorsForDraw(this);
    }
    
    _handle->DrawInstanced(vtx_count_per_instance, instance_count, start_vtx_location, start_instance_location);
}


void 
CommandList::DrawIndexedInstanced(UINT index_count_per_instance,
                                  UINT instance_count,
                                  UINT start_index_location,
                                  INT  base_vertex_location ,
                                  UINT start_instance_location)
{
    FlushResourceBarriers();
    
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        _dynamic_descriptor_heap[i].CommitStagedDescriptorsForDraw(this);
    }
    
    _handle->DrawIndexedInstanced(index_count_per_instance, 
                                  instance_count, 
                                  start_index_location, 
                                  base_vertex_location, 
                                  start_instance_location);
}

void 
CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, ID3D12DescriptorHeap* heap)
{
    if (_descriptor_heaps[heap_type] != heap )
    {
        _descriptor_heaps[heap_type] = heap;
        BindDescriptorHeaps();
    }
}

void 
CommandList::BindDescriptorHeaps()
{
    UINT num_descriptor_heaps = 0;
    ID3D12DescriptorHeap* descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
    
    for (u32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        ID3D12DescriptorHeap* descriptor_heap = _descriptor_heaps[i];
        if (descriptor_heap)
        {
            descriptor_heaps[num_descriptor_heaps++] = descriptor_heap;
        }
    }
    
    _handle->SetDescriptorHeaps(num_descriptor_heaps, descriptor_heaps);
}

void 
CommandList::TrackObject(ID3D12Object* object)
{
    arrput(_tracked_objects, object);
}

void 
CommandList::TrackResource(ID3D12Resource *resource)
{
    TrackObject(resource);
}

void 
CommandList::ReleaseTrackedObjects()
{
    for (u32 i = 0; i < (u32)arrlen(_tracked_objects); ++i)
    {
        // NOTE(Dustin): Might not be what we want to do
        //D3D_RELEASE(_tracked_objects[i]);
        _tracked_objects[i] = nullptr;
    }
    arrsetlen(_tracked_objects, 0);
}