
void 
RenderTarget::Init()
{
    _width = 0;
    _height = 0;
    memset(_textures, (int)INVALID_TEXTURE_ID.val, sizeof(TEXTURE_ID) * NumAttachmentPoints);
}

void 
RenderTarget::Free()
{
    _width = 0;
    _height = 0;
    memset(_textures, (int)INVALID_TEXTURE_ID.val, sizeof(TEXTURE_ID) * NumAttachmentPoints);
}

// Can specify the texture as null in order to remove the texture
void
RenderTarget::AttachTexture(AttachmentPoint slot, TEXTURE_ID texture_id)
{
    _textures[slot] = texture_id;
    if (texture::IsValid(texture_id))
    {
        Texture *texture = texture::GetTexture(texture_id);
        
        D3D12_RESOURCE_DESC desc = texture->_resource.GetResourceDesc();
        if (_width == 0 || _height == 0)
        { // RT width/height has not been intialized
            _width = (u32)desc.Width;
            _height = (u32)desc.Height;
        }
        else if (desc.Width != _width || desc.Height != _height)
        {
            texture->Resize(_width, _height);
        }
    }
}

TEXTURE_ID
RenderTarget::GetTexture(AttachmentPoint slot)
{
    return _textures[slot];
}

void 
RenderTarget::Resize(u32 width, u32 height)
{
    if (_width != width && _height != height)
    {
        _width = width;
        _height = height;
        
        for (u32 i = 0; i < NumAttachmentPoints; ++i)
        {
            if (texture::IsValid(_textures[i]))
            {
                Texture *texture = texture::GetTexture(_textures[i]);
                texture->Resize(_width, _height);
            }
        }
    }
}

// Get Aa viewport for this render target
// Scale & bias parameters can be used to specify a split screen viewport
// (bias parameter is normalized in the range [0...1]). By default, fullscreen
// is return.
D3D12_VIEWPORT 
RenderTarget::GetViewport(v3 scale, v2 bias, r32 min_depth, r32 max_depth)
{
    u32 width = 0, height = 0;
    
    for (i32 i = 0; i < AttachmentPoint::Color7; ++i)
    {
        if (texture::IsValid(_textures[i]))
        {
            D3D12_RESOURCE_DESC desc = texture::GetResourceDesc(_textures[i]);
            width = fast_max(width, (int)desc.Width);
            height = fast_max(height, (int)desc.Height);
        }
    }
    
    D3D12_VIEWPORT viewport = {
        ( width  * bias.x ),   // TopLeftX
        ( height * bias.y ),   // TopLeftY
        ( width  * scale.x ),  // Width
        ( height * scale.y ),  // Height
        min_depth,             // MinDepth
        max_depth              // MaxDepth
    };
    
    return viewport;
}

// Get the Render Target formats of the textures currently attached
// to this render target object. This is needed to configure the 
// pipeline state object.
D3D12_RT_FORMAT_ARRAY 
RenderTarget::GetRenderTargetFormats()
{
    D3D12_RT_FORMAT_ARRAY result = {};
    
    for (i32 i = 0; i < AttachmentPoint::Color7; ++i)
    {
        if (texture::IsValid(_textures[i]))
        {
            Texture *texture = texture::GetTexture(_textures[i]);
            result.RTFormats[result.NumRenderTargets++] = texture::GetResourceDesc(_textures[i]).Format;
        }
    }
    
    return result;
}

DXGI_FORMAT 
RenderTarget::GetDepthStencilFormat()
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
    
    if (texture::IsValid(_textures[AttachmentPoint::DepthStencil]))
    {
        result = texture::GetResourceDesc(_textures[AttachmentPoint::DepthStencil]).Format;
    }
    
    return result;
}

DXGI_SAMPLE_DESC 
RenderTarget::GetSampleDesc()
{
    DXGI_SAMPLE_DESC result = { 1, 0 };
    for (i32 i = 0; i < AttachmentPoint::Color7; ++i)
    {
        if (texture::IsValid(_textures[i]))
        {
            result = texture::GetResourceDesc(_textures[i]).SampleDesc;
        }
    }
    return result;
}

// Reset the render target - clears all textures...
void 
RenderTarget::Reset()
{
    memset(_textures, (int)INVALID_TEXTURE_ID.val, sizeof(TEXTURE_ID) * NumAttachmentPoints);
}
