
namespace d3d_pso
{
    // SOURCE for constants: 
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.cpp
    
    // Counter-clockwise vertex winding
    
    static const D3D12_RASTERIZER_DESC RasterizerDefault = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_NONE,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerDefaultMsaa = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_BACK,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        TRUE,                                      // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    // Clockwise winding vertex winding
    
    static const D3D12_RASTERIZER_DESC RasterizerDefaultCw = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_NONE,                      // Cull mode
        FALSE,                                     // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_BACK,                      // Cull mode
        FALSE,                                     // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        TRUE,                                      // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    // CCW Two Sided
    
    static const D3D12_RASTERIZER_DESC RasterizerTwoSided = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_NONE,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_NONE,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        TRUE,                                      // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerShadow = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_BACK,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        -100,                                      // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        -1.5,                                      // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerShadowCW = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_BACK,                      // Cull mode
        FALSE,                                     // Winding order (true == ccw)
        -100,                                      // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        -1.5,                                      // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_RASTERIZER_DESC RasterizerShadowTwoSided = {
        D3D12_FILL_MODE_SOLID,                     // Fill Mode
        D3D12_CULL_MODE_NONE,                      // Cull mode
        TRUE,                                      // Winding order (true == ccw)
        -100,                                      // Depth Bias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
        -1.5,                                      // Slope scaled depth bias
        TRUE,                                      // Depth Clip Enable
        FALSE,                                     // Multisample Enable
        FALSE,                                     // Antialiased enable
        0,                                         // Forced sample count
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
    static const D3D12_DEPTH_STENCIL_DESC DepthStateDisabled = {
        FALSE,                            // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
        D3D12_COMPARISON_FUNC_ALWAYS,     // DepthFunc
        FALSE,                            // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
        {                                 // FrontFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
        {                                 // BackFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
    };
    
    static const D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite = {
        TRUE,                             // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ALL,       // DepthWriteMask
        D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
        FALSE,                            // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
        {                                 // FrontFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
        {                                 // BackFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
    };
    
    static const D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly = {
        TRUE,                             // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
        D3D12_COMPARISON_FUNC_GREATER_EQUAL, // DepthFunc
        FALSE,                            // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
        {                                 // FrontFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
        {                                 // BackFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
    };
    
    static const D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed = {
        TRUE,                             // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
        D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
        FALSE,                            // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
        {                                 // FrontFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
        {                                 // BackFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
    };
    
    static const D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual = {
        TRUE,                             // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
        D3D12_COMPARISON_FUNC_EQUAL,      // DepthFunc
        FALSE,                            // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
        {                                 // FrontFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
        {                                 // BackFace
            D3D12_STENCIL_OP_KEEP,            // StencilFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,            // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
        },
    };
    
    // alpha blend
    static const D3D12_BLEND_DESC BlendNoColorWrite = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                FALSE,                             // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_SRC_ALPHA,             // SrcBlend
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                0,                                 // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
    
    static const D3D12_BLEND_DESC BlendDisable = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                FALSE,                             // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_SRC_ALPHA,             // SrcBlend
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
    
    static const D3D12_BLEND_DESC BlendTraditional = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                TRUE,                              // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_SRC_ALPHA,             // SrcBlend
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
    
    static const D3D12_BLEND_DESC BlendPreMultiplied = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                TRUE,                              // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_ONE,                   // SrcBlend
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
    
    static const D3D12_BLEND_DESC BlendAdditive = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                TRUE,                              // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_ONE,                   // SrcBlend
                D3D12_BLEND_ONE,                   // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
    
    static const D3D12_BLEND_DESC BlendTraditionalAdditive = {
        FALSE,                             // AlphaToCoverageEnable
        FALSE,                             // IndependentBlendEnable
        {                                  // RenderTarget[8]
            {                                  // index = 0
                TRUE,                              // BlendEnable
                FALSE,                             // LogicOpEnable
                D3D12_BLEND_SRC_ALPHA,             // SrcBlend
                D3D12_BLEND_ONE,                   // DestBlend
                D3D12_BLEND_OP_ADD,                // BlendOp
                D3D12_BLEND_ONE,                   // SrcBlendAlpha
                D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
                D3D12_BLEND_OP_ADD,                // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,               // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
            }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
        },
    };
};


// Get a default description of the rasterizer state
static D3D12_RASTERIZER_DESC 
GetRasterizerState(RasterState type)
{
    D3D12_RASTERIZER_DESC result = {};
    
    if (type == RasterState::Default)
        result = d3d_pso::RasterizerDefault;
    else if (type == RasterState::DefaultMSAA)
        result = d3d_pso::RasterizerDefaultMsaa;
    else if (type == RasterState::DefaultCw)
        result = d3d_pso::RasterizerDefaultCw;
    else if (type == RasterState::DefaultCwMSAA)
        result = d3d_pso::RasterizerDefaultCwMsaa;
    else if (type == RasterState::TwoSided)
        result = d3d_pso::RasterizerTwoSided;
    else if (type == RasterState::TwoSidedMSAA)
        result = d3d_pso::RasterizerTwoSidedMsaa;
    else if (type == RasterState::Shadow)
        result = d3d_pso::RasterizerShadow;
    else if (type == RasterState::ShadowCW)
        result = d3d_pso::RasterizerShadowCW;
    else if (type == RasterState::ShadowTwoSided)
        result = d3d_pso::RasterizerShadowTwoSided;
    
    return result;
}

// Get a default description of the depth-stencil state
static D3D12_DEPTH_STENCIL_DESC 
GetDepthStencilState(DepthStencilState type)
{
    D3D12_DEPTH_STENCIL_DESC result = {};
    
    if (type == DepthStencilState::Disabled)
        result = d3d_pso::DepthStateDisabled;
    else if (type == DepthStencilState::ReadWrite)
        result = d3d_pso::DepthStateReadWrite;
    else if (type == DepthStencilState::ReadOnly)
        result = d3d_pso::DepthStateReadOnly;
    else if (type == DepthStencilState::ReadOnlyReversed)
        result = d3d_pso::DepthStateReadOnlyReversed;
    else if (type == DepthStencilState::TestEqual)
        result = d3d_pso::DepthStateTestEqual;
    
    return result;
}

// Get a default description of the blend state
static D3D12_BLEND_DESC 
GetBlendState(BlendState type)
{
    D3D12_BLEND_DESC result = {};
    
    if (type == BlendState::Disabled)
        result = d3d_pso::BlendDisable;
    else if (type == BlendState::NoColorWrite)
        result = d3d_pso::BlendNoColorWrite;
    else if (type == BlendState::Traditional)
        result = d3d_pso::BlendTraditional;
    else if (type == BlendState::PreMultiplied)
        result = d3d_pso::BlendPreMultiplied;
    else if (type == BlendState::Additive)
        result = d3d_pso::BlendAdditive;
    else if (type == BlendState::TraditionalAdditive)
        result = d3d_pso::BlendTraditionalAdditive;
    
    return result;
}

struct GfxShaderBlob*
LoadShaderModule(wchar_t *file)
{
    struct GfxShaderBlob* result = 0;
    AssertHr(D3DReadFileToBlob(file, (ID3DBlob**)&result));
    return result;
}

FORCE_INLINE D3D12_FILL_MODE
ToD3DFillMode(GfxFillMode mode)
{
    D3D12_FILL_MODE result = D3D12_FILL_MODE_SOLID;
    if (mode == GfxFillMode::Wireframe)
        result = D3D12_FILL_MODE_WIREFRAME;
    return result;
}

FORCE_INLINE D3D12_CULL_MODE
ToD3DCullMode(GfxCullMode mode)
{
    D3D12_CULL_MODE result = D3D12_CULL_MODE_NONE;
    if (mode == GfxCullMode::Front)
        result = D3D12_CULL_MODE_FRONT;
    else if (mode == GfxCullMode::Back)
        result = D3D12_CULL_MODE_BACK;
    return result;
}

FORCE_INLINE DXGI_FORMAT
ToDXGIFormat(GfxFormat format)
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
    if (format == GfxFormat::R32_Float)
        result = DXGI_FORMAT_R32_FLOAT;
    else if (format == GfxFormat::R32G32_Float)
        result = DXGI_FORMAT_R32G32_FLOAT;
    else if (format == GfxFormat::R32G32B32_Float)
        result = DXGI_FORMAT_R32G32B32_FLOAT;
    else if (format == GfxFormat::R32G32B32A32_Float)
        result = DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (format == GfxFormat::D32_Float)
        result = DXGI_FORMAT_D32_FLOAT;
    else if (format == GfxFormat::R8G8B8A8_Unorm)
        result = DXGI_FORMAT_R8G8B8A8_UNORM;
    //else if (format == GfxFormat::Swapchain_DSV)
    //result = depth_buffer_format;
    else
    {
        assert(false);
    }
    return result;
}

FORCE_INLINE D3D12_INPUT_CLASSIFICATION 
ToD3DInputClass(GfxInputClass input)
{
    D3D12_INPUT_CLASSIFICATION result = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    if (input == GfxInputClass::PerInstance)
        result = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    return result;
}

FORCE_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE 
ToD3DTopologyType(GfxTopology top)
{
    D3D12_PRIMITIVE_TOPOLOGY_TYPE result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    if (top == GfxTopology::Point)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    else if (top == GfxTopology::Line)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    else if (top == GfxTopology::Triangle)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    else if (top == GfxTopology::Patch)
        result = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    return result;
}

void 
PipelineStateObject::Init(GfxPipelineStateDesc *pso_desc)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = pso_desc->root_signature->_handle;
    psoDesc.SampleMask     = UINT_MAX;
    psoDesc.DepthStencilState = pso_desc->depth;
    psoDesc.RasterizerState   = pso_desc->rasterizer;
    psoDesc.BlendState        = pso_desc->blend;
    
    for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        psoDesc.BlendState.RenderTarget[i] = psoDesc.BlendState.RenderTarget[0];
    
    // @MALLOC
    D3D12_INPUT_ELEMENT_DESC *InputElementDescs = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * pso_desc->input_layouts_count);
    for (u32 i = 0; i < pso_desc->input_layouts_count; ++i)
    {
        GfxInputElementDesc *input = &pso_desc->input_layouts[i];
        InputElementDescs[i] = {
            input->semantic_name, 
            input->semantic_index,
            ToDXGIFormat(input->format),
            input->input_slot,
            input->aligned_byte_offset,
            ToD3DInputClass(input->input_class),
            input->input_step_rate,
        };
    }
    psoDesc.InputLayout = { InputElementDescs, pso_desc->input_layouts_count };
    
#define SetShaderModule(mod) {                                            \
reinterpret_cast<UINT8*>(((ID3DBlob*)(mod))->GetBufferPointer()), \
((ID3DBlob*)(mod))->GetBufferSize()                               \
}
    
    GfxShaderModules *modules = &pso_desc->shader_modules;
    if (modules->vertex)
        psoDesc.VS = SetShaderModule(modules->vertex);
    if (modules->pixel)
        psoDesc.PS = SetShaderModule(modules->pixel);
    if (modules->hull)
        psoDesc.HS = SetShaderModule(modules->hull);
    if (modules->domain)
        psoDesc.DS = SetShaderModule(modules->domain);
    if (modules->geometry)
        psoDesc.GS = SetShaderModule(modules->geometry);
    
#undef SetShaderModule
    
    psoDesc.DSVFormat = ToDXGIFormat(pso_desc->dsv_format);
    psoDesc.PrimitiveTopologyType = ToD3DTopologyType(pso_desc->topology);
    psoDesc.SampleDesc.Count = pso_desc->sample_desc.count;
    psoDesc.SampleDesc.Quality = pso_desc->sample_desc.quality;
    
    psoDesc.NumRenderTargets = pso_desc->rtv_formats.NumRenderTargets;
    for (u32 i = 0; i < psoDesc.NumRenderTargets; ++i)
        psoDesc.RTVFormats[i] = pso_desc->rtv_formats.RTFormats[i];
    
    AssertHr(d3d_device->CreateGraphicsPipelineState(&psoDesc, IIDE(&_handle)));
    
    free(InputElementDescs);
}

RenderError
PipelineStateObject::Free()
{
    RenderError result = RenderError::Success;
    if (_handle) D3D_RELEASE(_handle);
    return result;
}