
namespace device
{
    static IDXGIAdapter1      *g_adapter = 0;
    static ID3D12Device       *g_device  = 0;
    
    static const u32           g_adapter_id_override = UINT_MAX;
    static D3D_FEATURE_LEVEL   g_d3d_min_feature_level = D3D_FEATURE_LEVEL_11_0;
    static D3D_FEATURE_LEVEL   g_d3d_feature_level     = D3D_FEATURE_LEVEL_11_0;
    
    static CommandQueue        g_direct_command_queue;
    static CommandQueue        g_copy_command_queue;
    static CommandQueue        g_compute_command_queue;
    
    static DescriptorAllocator g_descriptor_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    
    static void CreateAdapter();
    static void FreeAdapter();
    static void EnableDebugLayer();
    static void ReportLiveObjects();
};

static void
device::EnableDebugLayer()
{
    ID3D12Debug *debugController;
    AssertHr(D3D12GetDebugInterface(IIDE(&debugController)));
    debugController->EnableDebugLayer();
}

static void 
device::ReportLiveObjects()
{
    IDXGIDebug1* dxgi_debug;
    DXGIGetDebugInterface1(0, IIDE(&dxgi_debug ));
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgi_debug->Release();
}

static void 
device::CreateAdapter()
{
    IDXGIFactory6 *factory6;
    IDXGIAdapter1 *dxgi_adapter;
    
    UINT create_factory_flags = 0;
#if defined(_DEBUG)
    create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    
    AssertHr(CreateDXGIFactory2(create_factory_flags, IIDE(&factory6)));
    
    // Determines whether tearing support is available for fullscreen borderless windows.
    if (options & (c_allow_tearing | c_require_tearing_support))
    {
        BOOL allowTearing = FALSE;
        
        IDXGIFactory5 *factory5;
        HRESULT hr = factory6->QueryInterface(IIDE(&factory5));
        
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }
        
        if (FAILED(hr) || !allowTearing)
        {
            //mprinte("WARNING: Variable refresh rate displays are not supported.\n");
            assert((options & c_require_tearing_support) == 0);
#if 0
            if (options & c_require_tearing_support)
            {
                result = RenderError::TearingUnavailable;
            }
#endif
            options &= ~c_allow_tearing;
        }
    }
    
    // Initialize DXGI Adapter
    
    for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(adapterID, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IIDE(&dxgi_adapter)); ++adapterID)
    {
        if (g_adapter_id_override != UINT_MAX && adapterID != g_adapter_id_override)
        {
            continue;
        }
        
        DXGI_ADAPTER_DESC1 desc;
        AssertHr(dxgi_adapter->GetDesc1(&desc));
        
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }
        
        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(dxgi_adapter, g_d3d_min_feature_level, _uuidof(ID3D12Device), nullptr)))
        {
#if 0
            adapter_id = adapterID;
            i64 len = StrLen16((char16_t*)desc.Description);
            adapter_description = (char16_t*)MemAlloc(sizeof(char16_t) * (u32)len+1);
            memcpy(adapter_description, desc.Description, len * sizeof(char16_t));
            adapter_description[len] = 0;
#endif
            break;
        }
    }
    
#if !defined(NDEBUG)
    if (!dxgi_adapter && g_adapter_id_override == UINT_MAX)
    {
        // Try WARP12 instead
        AssertHr(factory6->EnumWarpAdapter(IIDE(&dxgi_adapter)));
    }
#endif
    
    assert(dxgi_adapter);
    g_adapter = dxgi_adapter;
}

static void
device::CreateDevice()
{
#if defined( _DEBUG )
    EnableDebugLayer();
#endif
    
    CreateAdapter();
    
    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    
    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };
    
    AssertHr(D3D12CreateDevice(g_adapter, g_d3d_min_feature_level, IIDE(&g_device)));
    
    // Configure debug device (if active).
    ID3D12InfoQueue *d3dInfoQueue;
    if (SUCCEEDED(g_device->QueryInterface(IIDE(&d3dInfoQueue))))
    {
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        
        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->PushStorageFilter(&filter);
    }
    
    // Get the Max supported feature level
    HRESULT hr = g_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        g_d3d_feature_level = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        g_d3d_feature_level = g_d3d_min_feature_level;
    }
    
    // Create descriptor allocators
    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        g_descriptor_allocators[i].Init((D3D12_DESCRIPTOR_HEAP_TYPE)(i));
    }
    
    // Create command queues
    g_direct_command_queue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_copy_command_queue.Init(D3D12_COMMAND_LIST_TYPE_COPY);
    g_compute_command_queue.Init(D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

static void
device::FreeAdapter()
{
    D3D_RELEASE(g_adapter);
}

static void
device::FreeDevice()
{
    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        g_descriptor_allocators[i].Free();
    }
    
    g_direct_command_queue.Free();
    g_copy_command_queue.Free();
    g_compute_command_queue.Free();
    
    FreeAdapter();
    D3D_RELEASE(g_device);
}


static bool 
device::AllowTearing(u32 options)
{
    return options & c_allow_tearing;
}


static DescriptorAllocation
device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count)
{
    return g_descriptor_allocators[type].Allocate(count);
}

static void
device::ReleaseStaleDescriptors()
{
    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        g_descriptor_allocators[i].ReleaseStaleDescriptors();
    }
}

static CommandQueue* 
device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    CommandQueue *result = 0;
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:  result = &g_direct_command_queue;  break;
        case D3D12_COMMAND_LIST_TYPE_COPY:    result = &g_copy_command_queue;    break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: result = &g_compute_command_queue; break;
    }
    return result;
}

static ID3D12Device*
device::GetDevice()
{
    return g_device;
}

static IDXGIAdapter1* 
device::GetAdapter()
{
    return g_adapter;
}

static void
device::Flush()
{
    g_direct_command_queue.Flush();
    g_copy_command_queue.Flush();
    g_compute_command_queue.Flush();
}

static u32 
device::MaxBackBufferCount()
{
    return MAX_BACK_BUFFER_COUNT;
}

DXGI_SAMPLE_DESC device::GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples,
                                                     D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags)
{
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
    
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format           = format;
    qualityLevels.SampleCount      = 1;
    qualityLevels.Flags            = flags;
    qualityLevels.NumQualityLevels = 0;
    
    while (qualityLevels.SampleCount <= numSamples &&
           SUCCEEDED(g_device->CheckFeatureSupport( D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels,
                                                   sizeof( D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ) ) ) &&
           qualityLevels.NumQualityLevels > 0 )
    {
        // That works...
        sampleDesc.Count   = qualityLevels.SampleCount;
        sampleDesc.Quality = qualityLevels.NumQualityLevels - 1;
        
        // But can we do better?
        qualityLevels.SampleCount *= 2;
    }
    
    return sampleDesc;
}