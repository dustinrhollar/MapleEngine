
namespace texture {
    
    static const u32   MIN_TEX_FREE_INDICES = 50; 
    static Texture    *g_texture_storage = 0;
    static u8         *g_texture_gen     = 0;
    static u32        *g_free_indices    = 0;
    // hashtable that maps a filename to a texture id
    // TODO(Dustin):
    static TEXTURE_ID *g_texture_cache = 0;
    
    static TEXTURE_ID GenTexId();
    
    static TEXTURE_ID LoadFromFile(const char *filename);
    
}; // texture

static TEXTURE_ID
texture::Create(D3D12_RESOURCE_DESC *rsrc_desc, D3D12_CLEAR_VALUE *clear_val)
{
    TEXTURE_ID result = GenTexId();
    g_texture_storage[result.idx].Init(rsrc_desc, clear_val);
    return result;
}

static TEXTURE_ID 
texture::Create(ID3D12Resource *rsrc, D3D12_CLEAR_VALUE *clear_val)
{
    TEXTURE_ID result = GenTexId();
    g_texture_storage[result.idx].Init(rsrc, clear_val);
    //ResourceStateTracker::AddGlobalResourceState(g_texture_storage[result.idx]._resource._handle, 
    //D3D12_RESOURCE_STATE_COMMON);
    return result;
}

static void 
texture::SafeFree(TEXTURE_ID tex)
{
    if (IsValid(tex))
    {
        Texture *texture = GetTexture(tex);
        texture->SafeFree();
        g_texture_gen[tex.idx] += 1;     // invalidate the texture id
        arrput(g_free_indices, tex.idx); // mark the slot as available
    }
}

static void
texture::Free(TEXTURE_ID tex)
{
    if (IsValid(tex))
    {
        Texture *texture = GetTexture(tex);
        texture->Free();
        g_texture_gen[tex.idx] += 1;     // invalidate the texture id
        arrput(g_free_indices, tex.idx); // mark the slot as available
    }
}

static TEXTURE_ID 
texture::GenTexId()
{
    TEXTURE_ID result = {};
    
    if (arrlen(g_free_indices) > MIN_TEX_FREE_INDICES)
    {
        result.idx = g_free_indices[0];
        result.gen = g_texture_gen[result.idx];
        arrdel(g_free_indices, 0);
    }
    else
    {
        result.idx = arrlen(g_texture_gen);
        result.gen = 0;
        
        arrput(g_texture_gen, result.gen);
        // Put dummy texture into storage
        Texture tex = {}; arrput(g_texture_storage, tex);
    }
    
    return result;
}

static void 
texture::Reset(TEXTURE_ID tex_id)
{
    if (IsValid(tex_id))
    {
        g_texture_storage[tex_id.idx]._resource.Reset();
    }
}

static bool 
texture::IsValid(TEXTURE_ID tex)
{
    return (tex.val != INVALID_TEXTURE_ID.val) && (tex.idx < arrlen(g_texture_gen)) && (tex.gen == g_texture_gen[tex.idx]);
}

static D3D12_RESOURCE_DESC 
texture::GetResourceDesc(TEXTURE_ID tex_id)
{
    if (IsValid(tex_id))
    {
        return g_texture_storage[tex_id.idx]._resource.GetResourceDesc();
    }
    else
    {
        return {};
    }
}

static void
texture::Resize(TEXTURE_ID tex_id, u32 width, u32 height, u32 depthOrArraySize)
{
    if (IsValid(tex_id))
    {
        Texture *texture = g_texture_storage + tex_id.idx;
        D3D12_RESOURCE_DESC resDesc = texture->_resource.GetResourceDesc();
        if (width != resDesc.Width || height != resDesc.Height || depthOrArraySize != resDesc.DepthOrArraySize)
        {
            texture->Resize(width, height, depthOrArraySize);
        }
    }
}

static Resource* 
texture::GetResource(TEXTURE_ID tex_id)
{
    Resource* result = 0;
    if (IsValid(tex_id))
    {
        result = &g_texture_storage[tex_id.idx]._resource;
    }
    return result;
}

static Texture* 
texture::GetTexture(TEXTURE_ID tex_id)
{
    Texture* result = 0;
    if (IsValid(tex_id))
    {
        result = &g_texture_storage[tex_id.idx];
    }
    return result;
}
static void 
texture::SetName(TEXTURE_ID tex_id, const wchar_t *name)
{
    if (IsValid(tex_id))
    {
        Texture* texture = &g_texture_storage[tex_id.idx];
        texture->_resource._handle->SetName(name);
    }
}



static D3D12_UNORDERED_ACCESS_VIEW_DESC
GetUAVDesc(D3D12_RESOURCE_DESC* resDesc, UINT mipSlice, UINT arraySlice = 0,
           UINT planeSlice = 0)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format                           = resDesc->Format;
    
    switch (resDesc->Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        { 
            if (resDesc->DepthOrArraySize > 1)
            {
                uavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                uavDesc.Texture1DArray.ArraySize       = resDesc->DepthOrArraySize - arraySlice;
                uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture1DArray.MipSlice        = mipSlice;
            }
            else
            {
                uavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                uavDesc.Texture1D.MipSlice = mipSlice;
            }
        } break;
        
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        {
            if ( resDesc->DepthOrArraySize > 1 )
            {
                uavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize       = resDesc->DepthOrArraySize - arraySlice;
                uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture2DArray.PlaneSlice      = planeSlice;
                uavDesc.Texture2DArray.MipSlice        = mipSlice;
            }
            else
            {
                uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Texture2D.PlaneSlice = planeSlice;
                uavDesc.Texture2D.MipSlice   = mipSlice;
            }
        } break;
        
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        {
            uavDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.WSize       = resDesc->DepthOrArraySize - arraySlice;
            uavDesc.Texture3D.FirstWSlice = arraySlice;
            uavDesc.Texture3D.MipSlice    = mipSlice;
        } break;
        
        default: assert(false && "Invalid resource dimension.");
    }
    
    return uavDesc;
}

void 
Texture::Init(D3D12_RESOURCE_DESC *rsrc_desc, D3D12_CLEAR_VALUE *clear_val)
{
    _srv = {};
    _dsv = {};
    _rtv = {};
    _uav = {};
    _resource.Init(rsrc_desc, clear_val);
    CreateViews();
}

void 
Texture::Init(ID3D12Resource *rsrc, D3D12_CLEAR_VALUE *clear_val)
{
    _srv = {};
    _dsv = {};
    _rtv = {};
    _uav = {};
    _resource.Init(rsrc, clear_val);
    CreateViews();
}

void 
Texture::SafeFree()
{
    _rtv.Free();
    _dsv.Free();
    _srv.Free();
    _uav.Free();
    
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
    _rtv = {};
    _dsv = {};
    _srv = {};
    _uav = {};
    _resource = {};
}

void 
Texture::Free()
{
    _rtv.Free();
    _dsv.Free();
    _srv.Free();
    _uav.Free();
    _resource.Free();
    
    _rtv = {};
    _dsv = {};
    _srv = {};
    _uav = {};
    _resource = {};
}

void 
Texture::Resize(u32 width, u32 height, u32 depthOrArraySize)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    if (_resource.IsValid())
    {
        // NOTE(Dustin): Remove the previous resource from global state?
        // D3D_RELEASE the previous resource handle?
        D3D12_RESOURCE_DESC resDesc = _resource.GetResourceDesc();
        
        resDesc.Width            = fast_max(width, 1);
        resDesc.Height           = fast_max(height, 1);
        resDesc.DepthOrArraySize = depthOrArraySize;
        resDesc.MipLevels        = resDesc.SampleDesc.Count > 1 ? 1 : 0;
        
        D3D12_CLEAR_VALUE *clear_val;
        _resource.GetClearValue(&clear_val);
        
        // Garbage collect the resource
        CommandList *active_list = RendererGetActiveCommandList();
        active_list->DeleteResource(&_resource);
        ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
        
        D3D12_HEAP_PROPERTIES heap_prop = d3d::GetHeapProperties();
        AssertHr(d3d_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resDesc,
                                                     D3D12_RESOURCE_STATE_COMMON, clear_val, IIDE(&_resource._handle)));
        
        ResourceStateTracker::AddGlobalResourceState(_resource._handle, D3D12_RESOURCE_STATE_COMMON);
        
        CreateViews();
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE 
Texture::GetRenderTargetView()
{
    return _rtv.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE 
Texture::GetDepthStencilView()
{
    return _dsv.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE 
Texture::GetShaderResourceView()
{
    return _srv.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::
Texture::GetUnorderedAccessView(u32 mip)
{
    return _uav.GetDescriptorHandle();
}

// Check if the texture has alpha support
bool 
Texture::HasAlpha()
{
    DXGI_FORMAT format = _resource.GetResourceDesc().Format;
    
    bool hasAlpha = false;
    switch ( format )
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
        {
            hasAlpha = true;
        } break;
    }
    return hasAlpha;
}

u64 
Texture::BitsPerPixel()
{
    DXGI_FORMAT format = _resource.GetResourceDesc().Format;
    return texture::BitsPerPixel(format);
}

// @INTERNAL

// Create SRV & UAVS for the resource
void 
Texture::CreateViews()
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    if (_resource.IsValid())
    {
        D3D12_RESOURCE_DESC desc = _resource.GetResourceDesc();
        
        // Create RTV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport())
        {
            _rtv = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d3d_device->CreateRenderTargetView( _resource._handle, 0,
                                               _rtv.GetDescriptorHandle());
        }
        // Create DSV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport() )
        {
            _dsv = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            d3d_device->CreateDepthStencilView(_resource._handle, 0,
                                               _dsv.GetDescriptorHandle());
        }
        // Create SRV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && CheckSRVSupport())
        {
            _srv = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            d3d_device->CreateShaderResourceView( _resource._handle, 0,
                                                 _srv.GetDescriptorHandle());
        }
        
        // Create UAV for each mip (only supported for 1D and 2D textures).
        if ( ( desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) != 0 && CheckUAVSupport() &&
            desc.DepthOrArraySize == 1 )
        {
            _uav = device::AllocateDescriptors( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.MipLevels );
            for ( int i = 0; i < desc.MipLevels; ++i )
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = GetUAVDesc(&desc, i);
                d3d_device->CreateUnorderedAccessView(_resource._handle, nullptr, &uavDesc,
                                                      _uav.GetDescriptorHandle(i));
            }
        }
    }
}

// Return a typeless format from the given format.
DXGI_FORMAT 
Texture::GetTypelessFormat(DXGI_FORMAT format)
{
    DXGI_FORMAT typelessFormat = format;
    
    switch ( format )
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
        break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
        break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
        break;
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
        break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
        break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        break;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
        break;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        typelessFormat = DXGI_FORMAT_R32_TYPELESS;
        break;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
        break;
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        typelessFormat = DXGI_FORMAT_R16_TYPELESS;
        break;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        typelessFormat = DXGI_FORMAT_R8_TYPELESS;
        break;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
        break;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
        break;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
        break;
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
        break;
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
        break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
        break;
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
        break;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
        break;
    }
    
    return typelessFormat;
}

// Return an sRGB format in the same format family.
DXGI_FORMAT 
Texture::GetSRGBFormat(DXGI_FORMAT format)
{
    DXGI_FORMAT srgbFormat = format;
    switch ( format )
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        srgbFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        break;
        case DXGI_FORMAT_BC1_UNORM:
        srgbFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
        break;
        case DXGI_FORMAT_BC2_UNORM:
        srgbFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
        break;
        case DXGI_FORMAT_BC3_UNORM:
        srgbFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
        break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        srgbFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        break;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        srgbFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        break;
        case DXGI_FORMAT_BC7_UNORM:
        srgbFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
        break;
    }
    
    return srgbFormat;
}

DXGI_FORMAT 
Texture::GetUAVCompatableFormat(DXGI_FORMAT format)
{
    DXGI_FORMAT uavFormat = format;
    
    switch ( format )
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        uavFormat = DXGI_FORMAT_R32_FLOAT;
        break;
    }
    
    return uavFormat;
}

bool 
Texture::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
        return true;
        default:
        return false;
    }
}

bool 
Texture::IsSRGBFormat(DXGI_FORMAT format)
{
    switch ( format )
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
        default:
        return false;
    }}

bool
Texture::IsBGRFormat(DXGI_FORMAT format)
{
    switch ( format )
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
        default:
        return false;
    }
}

bool 
Texture::IsDepthFormat(DXGI_FORMAT format)
{
    switch ( format )
    {
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D16_UNORM:
        return true;
        default:
        return false;
    }
}

static u64 
texture::BitsPerPixel(DXGI_FORMAT format)
{
    switch ((int)(format))
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;
        
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        return 96;
        
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
        return 64;
        
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
        return 32;
        
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
        return 24;
        
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;
        
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
        return 12;
        
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        return 8;
        
        case DXGI_FORMAT_R1_UNORM:
        return 1;
        
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        return 4;
        
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;
        
        default:
        return 0;
    }
}
