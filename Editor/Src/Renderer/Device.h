#ifndef _DEVICE_H
#define _DEVICE_H

struct CommandQueue;

namespace device
{
    static const u32 c_allow_tearing           = 0x1;
    static const u32 c_require_tearing_support = 0x2;
    static const u64 MAX_BACK_BUFFER_COUNT     = 3;
    
    static void CreateDevice();
    static void FreeDevice();
    
    static bool AllowTearing(u32 options);
    static u32  MaxBackBufferCount();
    
    static DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count = 1);
    static void ReleaseStaleDescriptors();
    
    static ID3D12Device* GetDevice();
    static IDXGIAdapter1* GetAdapter();
    static CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
    
    DXGI_SAMPLE_DESC GetMultisampleQualityLevels(DXGI_FORMAT format, 
                                                 UINT numSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
                                                 D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);
    
    
    static void Flush();
};

#endif //_DEVICE_H
