#ifndef _TEXTURE_H
#define _TEXTURE_H

union TEXTURE_ID
{
    struct {
        u32 idx:24;
        u32 gen:8;
    };
    u32 val;
};
static const TEXTURE_ID INVALID_TEXTURE_ID = {0xFFFFFFFF};

struct Texture
{
    void Init(D3D12_RESOURCE_DESC *rsrc_desc, D3D12_CLEAR_VALUE *clear_val = 0);
    void Init(ID3D12Resource *rsrc, D3D12_CLEAR_VALUE *clear_val = 0);
    void Free();
    
    void Resize(u32 width, u32 height, u32 depthOrArraySize = 1);
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView();
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView();
    D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(u32 mip);
    
    bool CheckSRVSupport()
    {
        return _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
    }
    
    bool CheckRTVSupport()
    {
        return _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
    }
    
    bool CheckUAVSupport()
    {
        return _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
            _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
            _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }
    
    bool CheckDSVSupport()
    {
        return _resource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
    }
    
    // Check if the texture has alpha support
    bool HasAlpha();
    
    u64 BitsPerPixel();
    
    static bool IsUAVCompatibleFormat( DXGI_FORMAT format );
    static bool IsSRGBFormat( DXGI_FORMAT format );
    static bool IsBGRFormat( DXGI_FORMAT format );
    static bool IsDepthFormat( DXGI_FORMAT format );
    
    // Return a typeless format from the given format.
    static DXGI_FORMAT GetTypelessFormat( DXGI_FORMAT format );
    // Return an sRGB format in the same format family.
    static DXGI_FORMAT GetSRGBFormat( DXGI_FORMAT format );
    static DXGI_FORMAT GetUAVCompatableFormat( DXGI_FORMAT format );
    
    // @INTERNAL
    
    // Create SRV & UAVS for the resource
    void CreateViews();
    
    Resource             _resource;
    DescriptorAllocation _rtv; // render target view
    DescriptorAllocation _dsv; // depth stencil view
    DescriptorAllocation _srv; // shader resource view
    DescriptorAllocation _uav; // unordered access view 
};

namespace texture
{
    static TEXTURE_ID Create(D3D12_RESOURCE_DESC *rsrc_desc, D3D12_CLEAR_VALUE *clear_val = 0);
    static TEXTURE_ID Create(ID3D12Resource *rsrc, D3D12_CLEAR_VALUE *clear_val = 0);
    static void Free(TEXTURE_ID tex);
    
    static void  Reset(TEXTURE_ID tex_id);
    static bool  IsValid(TEXTURE_ID tex_id);
    
    static D3D12_RESOURCE_DESC GetResourceDesc(TEXTURE_ID tex_id);
    // Gets the Resource attached to the texture
    static struct Resource* GetResource(TEXTURE_ID tex_id);
    static struct Texture*  GetTexture(TEXTURE_ID tex_id);
    static void Resize(TEXTURE_ID tex_id, u32 width, u32 height, u32 depthOrArraySize = 1);
    
    static u64 BitsPerPixel(DXGI_FORMAT format);
};

#endif //_TEXTURE_H
