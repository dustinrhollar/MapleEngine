#ifndef _ROOT_SIGNATURE_H
#define _ROOT_SIGNATURE_H

struct RootSignature
{
    RenderError Init(UINT numParameters,
                     const D3D12_ROOT_PARAMETER1* _pParameters,
                     UINT numStaticSamplers = 0,
                     const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = nullptr,
                     D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
    
    RenderError Free();
    
    u32 GetDescriptorTableBitmask(D3D12_DESCRIPTOR_HEAP_TYPE heap_type);
    u32 GetNumDescriptors(u32 root_idx);
    
    // @INTERNAL
    
    ID3D12RootSignature       *_handle = 0;
    D3D12_ROOT_SIGNATURE_DESC1 _desc;
    // Need to know number of descriptors per table
    //  A maximum of 32 descriptor tables are supported
    u32                        _num_descriptors_per_table[32];
    // A bitmask that represents the root parameter indices that 
    // are descriptor tables for Samplers
    u32                        _sampler_table_bitmask;
    // A bitmask that represents the root parameter indices
    // that are CBV/UAV/SRV descriptor tables
    u32                        _descriptor_table_bitmask;
};

#endif //_ROOT_SIGNATURE_H
