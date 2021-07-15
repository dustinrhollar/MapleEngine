#ifndef _PIPELINE_STATE_H
#define _PIPELINE_STATE_H

enum class GfxShaderStage : u8
{
    Vertex,
    Pixel,
    
    Count,
    Unknown = Count,
};

struct GfxShaderModules
{
    struct GfxShaderBlob *vertex    = 0;
    struct GfxShaderBlob *pixel     = 0;
    struct GfxShaderBlob *hull      = 0;
    struct GfxShaderBlob *domain    = 0;
    struct GfxShaderBlob *geometry  = 0;
    struct GfxShaderBlob *compute   = 0;
};

struct GfxShaderBlob
{
    ID3DBlob *handle;
};

enum class GfxFillMode
{
    Wireframe,
    Solid,
};

enum class GfxCullMode
{
    None,
    Front,
    Back,
};

struct GfxRasterDesc
{
    GfxFillMode fill_mode;
    GfxCullMode cull_mode;
    bool        front_count_clockwise;
    /* Omit depth bias? */
    /* Omit depth bias clamp? */
    /* Omit slope scaled depth bias? */
    bool       depth_clip_enabled;
    bool       multisample_enabled;
    bool       antialiased_enabled;
    u32        forced_sample_count;
    /* Omit conservative raster mode */
}; 

enum class GfxFormat : u8
{
    R32_Float,
    R32G32_Float,
    R32G32B32_Float,
    R32G32B32A32_Float,
    R8G8B8A8_Unorm,
    
    // TODO(Dustin): Others on a as-needed basis
    
    D32_Float, // depth format
};

enum class GfxInputClass : u8
{
    PerVertex,
    PerInstance,
};

enum class GfxTopology
{
    Undefined,
    Point,
    Line,
    Triangle,
    Patch,
};

/* Specifies multisampling settings */
struct GfxSampleDesc
{
    u32 count;
    u32 quality;
};

struct GfxInputElementDesc
{
    const char   *semantic_name;
    u32           semantic_index;
    GfxFormat     format;
    u32           input_slot;
    u32           aligned_byte_offset;
    GfxInputClass input_class;
    u32           input_step_rate;
};

struct GfxPipelineStateDesc
{
    RootSignature      *root_signature; // TODO(Dustin): Root sig id?
    GfxShaderModules    shader_modules;
    /* Omit stream output desc */
    /* Omit blend desc */
    /* Omit sample mask */
    //GfxRasterDesc       raster;
    /* Omit depth stencil state */
    // TODO(Dustin): Expose depth stencil
    GfxInputElementDesc *input_layouts;
    u32                  input_layouts_count;
    /* Omit index buffer strip cut value */
    GfxTopology         topology;
    
    D3D12_RASTERIZER_DESC    rasterizer;
    D3D12_DEPTH_STENCIL_DESC depth;
    D3D12_BLEND_DESC         blend;
    
#if 0
    /* When target count is 0, swapchain image is used by default */
    u32                 render_target_count;
    GfxFormat           rtv_formats[8];
#else
    D3D12_RT_FORMAT_ARRAY rtv_formats;
#endif
    GfxFormat           dsv_format;
    
    GfxSampleDesc       sample_desc;
    /* Omit node mask */
    /* Omit pipeline cache state */
    /* Omit Flags */
};

struct PipelineStateObject
{
    ID3D12PipelineState *_handle;
    
    void Init(GfxPipelineStateDesc *pso_desc);
    RenderError Free();
};

enum class RasterState : u8
{
    Default,
    DefaultMSAA,
    DefaultCw,
    DefaultCwMSAA,
    TwoSided,
    TwoSidedMSAA,
    Shadow,
    ShadowCW,
    ShadowTwoSided,
};

enum class DepthStencilState : u8
{
    Disabled,
    ReadWrite,
    ReadOnly,
    ReadOnlyReversed,
    TestEqual,
};

enum class BlendState : u8
{
    Disabled,
    NoColorWrite,
    Traditional,
    PreMultiplied,
    Additive,
    TraditionalAdditive,
};

// Get a default description of the rasterizer state
static D3D12_RASTERIZER_DESC GetRasterizerState(RasterState type);
// Get a default description of the depth-stencil state
static D3D12_DEPTH_STENCIL_DESC GetDepthStencilState(DepthStencilState type);
// Get a default description of the blend state
static D3D12_BLEND_DESC GetBlendState(BlendState type);

struct GfxShaderBlob* LoadShaderModule(wchar_t *file);



#endif //_PIPELINE_STATE_H
