#ifndef _RENDER_TARGET_H
#define _RENDER_TARGET_H

enum AttachmentPoint
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    NumAttachmentPoints,
};

struct RenderTarget
{
    void Init();
    void Free();
    
    // Can specify the texture as null in order to remove the texture
    void AttachTexture(AttachmentPoint slot, TEXTURE_ID texture);
    TEXTURE_ID GetTexture(AttachmentPoint slot);
    
    void Resize(u32 width, u32 height);
    
    // Get Aa viewport for this render target
    // Scale & bias parameters can be used to specify a split screen viewport
    // (bias parameter is normalized in the range [0...1]). By default, fullscreen
    // is return.
    D3D12_VIEWPORT GetViewport(v3 scale = {1.0f, 1.0f}, v2 bias = {0.0f, 0.0f},
                               r32 min_depth = 0.0f, r32 max_depth = 1.0f);
    
    // Get the Render Target formats of the textures currently attached
    // to this render target object. This is needed to configure the 
    // pipeline state object.
    D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats();
    
    DXGI_FORMAT GetDepthStencilFormat();
    
    DXGI_SAMPLE_DESC GetSampleDesc();
    
    // Reset the render target - clears all textures...
    void Reset();
    
    // @INTERNAL
    
    u32        _width;
    u32        _height;
    // Attached textures to the render target
    // NOTE(Dustin): The Render Target does not own the textures!
    TEXTURE_ID _textures[NumAttachmentPoints];
};

#endif //_RENDER_TARGET_H
