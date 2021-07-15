
//~ Implementation
static const u32 g_swapchain_buffer_count = 3;

void
Swapchain::Init(HWND hwnd, CommandQueue* present_queue, u32 width, u32 height)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    
    // Make sure the config is valid.
    //assert(config.buffer_count > 1 && config.buffer_count <= DXGI_MAX_SWAP_CHAIN_BUFFERS);
    //assert(config.sync_interval <= 4);
    //assert(config.max_frame_latency > 0);
    
    //swapchain->config = config;
    _vsync = true;
    _present_queue = present_queue;
    _render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    _render_target.Init();
    
    // Get the device that the command queue belongs to.
    ID3D12Device* device   = device::GetDevice();
    IDXGIAdapter1* adapter = device::GetAdapter();
    
    // Get the factory used to create te adapter
    IDXGIFactory  *dxgi_factory;
    AssertHr(adapter->GetParent(IIDE(&dxgi_factory)));
    
    // Check if we have tearing support. This is almost always true on Windows 10.
    BOOL allow_tearing = FALSE;
    IDXGIFactory5* factory5 = 0;
    if (SUCCEEDED(dxgi_factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&factory5)))
    {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing))))
        {
            _tearing_supported = FALSE;
        }
    }
    
    desc.Width = width;
    desc.Height = height;
    desc.Format = _render_target_format;
    desc.Stereo = FALSE;
    desc.SampleDesc = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = g_swapchain_buffer_count;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    
    u32 flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (_tearing_supported) flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    desc.Flags = flags;
    
    IDXGISwapChain1* swap_chain_1 = 0;
    AssertHr(factory5->CreateSwapChainForHwnd(present_queue->handle, hwnd, &desc, 0, 0, &swap_chain_1));
    
    // Disable the automatic Alt+Enter fullscreen toggle. We'll do this ourselves to support adaptive refresh rate stuff.
    AssertHr(factory5->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    
    // Cast to an IDXGISwapChain3.
    AssertHr(swap_chain_1->QueryInterface(_uuidof(IDXGISwapChain3), (void**)&_handle));
    swap_chain_1->Release();
    
    // Initialize all the RTVs and fence values to 0.
    for (u32 i = 0; i < DXGI_MAX_SWAP_CHAIN_BUFFERS; ++i)
    {
        _fence_values[i] = 0;
    }
    
    _frame_index = _handle->GetCurrentBackBufferIndex();
    _buffer_available_event = _handle->GetFrameLatencyWaitableObject();
    
    _handle->SetMaximumFrameLatency(g_swapchain_buffer_count - 1);
    device->Release();
    
    RECT windowRect;
    ::GetClientRect(hwnd, &windowRect);
    
    _width  = windowRect.right - windowRect.left;
    _height = windowRect.bottom - windowRect.top;
    
    UpdateRenderTargetViews();
}

void
Swapchain::Free()
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    AssertHr(_handle->GetDesc1(&desc));
    for (u32 i = 0; i < desc.BufferCount; ++i)
    {
        texture::Free(_back_buffer_textures[i]);
    }
    _render_target.Free();
    D3D_RELEASE(_handle);
    CloseHandle(_buffer_available_event);
    _present_queue = 0;
}

void
Swapchain::UpdateRenderTargetViews()
{
    for (u32 i = 0; i < g_swapchain_buffer_count; ++i)
    {
        ID3D12Resource* back_buffer;
        AssertHr(_handle->GetBuffer(i, IIDE(&back_buffer)));
        
        ResourceStateTracker::AddGlobalResourceState(back_buffer, D3D12_RESOURCE_STATE_COMMON);
        
        D3D12_CLEAR_VALUE clear_value;
        clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 1.0f;
        
        _back_buffer_textures[i] = texture::Create(back_buffer, &clear_value);
    }
}

RenderTarget* 
Swapchain::GetRenderTarget()
{
    _render_target.AttachTexture(AttachmentPoint::Color0, _back_buffer_textures[_frame_index]);
    return &_render_target;
}

u32
Swapchain::Present(TEXTURE_ID tex_id)
{
    CommandList *command_list = _present_queue->GetCommandList();
    
    TEXTURE_ID back_buffer_tex_id = _back_buffer_textures[_frame_index];
    Resource *back_buffer_texture = texture::GetResource(back_buffer_tex_id);
    
    if (texture::IsValid(tex_id))
    {
        Resource *cpy_texture = texture::GetResource(tex_id);
        D3D12_RESOURCE_DESC desc = cpy_texture->GetResourceDesc();
        if (desc.SampleDesc.Count > 1)
        {
            command_list->ResolveSubresource(back_buffer_texture, cpy_texture);
        }
        else
        {
            command_list->CopyResource(back_buffer_texture, cpy_texture);
        }
    }
    
    command_list->TransitionBarrier(back_buffer_texture->_handle, D3D12_RESOURCE_STATE_PRESENT);
    _present_queue->ExecuteCommandLists(&command_list, 1);
    
    UINT sync_interval = _vsync ? 1 : 0;
    u32 flags = 0;
    if (sync_interval == 0)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        AssertHr(_handle->GetDesc1(&desc));
        if ((desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0) flags |= DXGI_PRESENT_ALLOW_TEARING;
    }
    
    AssertHr(_handle->Present(sync_interval, flags));
    
    _fence_values[_frame_index] = _present_queue->Signal();
    _frame_index = _handle->GetCurrentBackBufferIndex();
    
    _present_queue->WaitForFenceValue(_fence_values[_frame_index]);
    device::ReleaseStaleDescriptors();
    
    return _frame_index;
}

void
Swapchain::Resize(u32 width, u32 height)
{
    //assert(swapchain->config.buffer_count > 1 && 
    //swapchain->config.buffer_count <= DXGI_MAX_SWAP_CHAIN_BUFFERS);
    
    if (_width != width || _height != height)
    {
        _width = fast_max(width, 1);
        _height = fast_max(height, 1);
        
        device::Flush();
        
        // Release all references to the backbuffer textures
        _render_target.Reset();
        _render_target.Resize(_width, _height);
        for (u32 i = 0; i < g_swapchain_buffer_count; ++i)
        {
            texture::Free(_back_buffer_textures[i]);
        }
        
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        AssertHr(_handle->GetDesc1(&desc));
        
        // We always want the waitable object flag, and the tearing flag if it is supported.
        u32 flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        if ((desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0) flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        AssertHr(_handle->ResizeBuffers(g_swapchain_buffer_count, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
        
        _frame_index = _handle->GetCurrentBackBufferIndex();
        UpdateRenderTargetViews();
    }
}
