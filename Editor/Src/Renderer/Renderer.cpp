
//#if !defined(MemAlloc) || !defined(MemFree)
//#define MemAlloc malloc
//#define MemFree  free
//#endif

//#include "Common/PlatformTypes.h"
//#include "Common/Core.h"
//#include "Platform/Platform.h"
//#include "Platform/HostKey.h"
//#include "Platform/HostWindow.h"
//#include "Common/Util/MapleMath.h"
// TODO(Dustin): Use Memory Manager instead...
//#include "Common/Util/stb_ds.h"
//#include "Common/Util/stb_image.h"

//#include <windows.h>
#include <dxgi.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define IIDE IID_PPV_ARGS

#define JMP_FAILED(f, e)   \
{                      \
HRESULT hr = (f);  \
if (FAILED(hr)) {  \
result = e;    \
goto LBL_FAIL; \
}                  \
}

#if defined(_DEBUG)
#define AssertHr(fn)         \
{                        \
HRESULT hr = (fn);    \
assert(SUCCEEDED(hr)); \
}
#else
#define AssertHr(fn) (f)
#endif

#define D3D_RELEASE(r) { (r)->Release(); (r) = 0; }

#include "RendererApi.h"

static UINT              back_buffer_count = 2;
// Per Frame Info
static u64               g_frame_count = 0;
// DeviceResources options (see flags in constructor)
static u32               options = 0;
// Callbacks
static RendererCallbacks g_callbacks{};

// Renderer Source

#include "Helpers.h"
#include "MemoryManagement.h"
#include "Device.h"
#include "ResourceStateTracker.h"
#include "Resource.h"
#include "Texture.h"
#include "RenderTarget.h"
#include "Swapchain.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "CommandList.h"
#include "CommandQueue.h"

#include "CommandQueue.cpp"
#include "Swapchain.cpp"
#include "RootSignature.cpp"
#include "Buffers.cpp"
#include "PipelineState.cpp"
#include "MemoryManagement.cpp"
#include "ResourceStateTracker.cpp"
#include "CommandList.cpp"
#include "Texture.cpp"
#include "Resource.cpp"
#include "Device.cpp"
#include "RenderTarget.cpp"
#include "ImGuiRenderer.cpp"

static Swapchain           g_swapchain{};      // @INTERNAL
// TODO(Dustin): When moving to a multithreaded renderer,
// should set a single command list for each thread
static CommandList *g_frame_command_list = 0;

#if 0

static RootSignature       g_root_signature{};
static VertexBuffer        g_vertex_buffer{};
static IndexBuffer         g_index_buffer{};
static PipelineStateObject g_pipeline_state{};

static TEXTURE_ID          g_color_texture = INVALID_TEXTURE_ID;
static TEXTURE_ID          g_depth_texture = INVALID_TEXTURE_ID;
static RenderTarget        g_render_target{};

// Vertex data for a colored cube.
struct VertexPosColor
{
    v3 Position;
    v3 Color;
    v2 TexCoords;
};

static VertexPosColor g_cube_vertices[8] = {
    { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // 0
    { { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } }, // 1
    { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }, // 2
    { {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } }, // 3
    { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // 4
    { { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }, // 5
    { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // 6
    { {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }  // 7
};

static u16 g_cube_indicies[36] = { 0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7 };

static TEXTURE_ID         g_diffuse_texture = INVALID_TEXTURE_ID;
static ShaderResourceView g_diffuse_srv = {};

#endif

// Predfined functions

FORCE_INLINE DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt);
FORCE_INLINE i64 StrLen16(char16_t *strarg);

static void HostWndSetWindowZorderToTopMost(HWND window, bool is_top);
//static RenderError CreateWindowSizeDependentResources();
static void WaitForGpu();
static RenderError BeginFrame(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
static RenderError EndFrame(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
static void HandleDeviceLost();
static RenderError PipelineSetup();

static void
RendererInit(RendererInitInfo *info)
{
    device::CreateDevice();
    
    InitalizeGlobalResourceState();
    
    CommandQueue *present_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_swapchain.Init((HWND)info->wnd, present_queue, info->wnd_width, info->wnd_height);
    
#if 0
    
    // Resize the depth texture.
    D3D12_RESOURCE_DESC depthTextureDesc = d3d::GetTex2DDesc(DXGI_FORMAT_D32_FLOAT, 
                                                             g_swapchain._width, 
                                                             g_swapchain._height);
    // Must be set on textures that will be used as a depth-stencil buffer.
    depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    // Specify optimized clear values for the depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil      = { 1.0F, 0 };
    
    g_depth_texture = texture::Create(&depthTextureDesc, &optimizedClearValue);
    
    DXGI_FORMAT back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(back_buffer_format, 
                                                           g_swapchain._width, 
                                                           g_swapchain._height);
    color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    color_tex_desc.MipLevels = 1;
    
    D3D12_CLEAR_VALUE color_clear_value;
    color_clear_value.Format   = color_tex_desc.Format;
    color_clear_value.Color[0] = 0.4f;
    color_clear_value.Color[1] = 0.6f;
    color_clear_value.Color[2] = 0.9f;
    color_clear_value.Color[3] = 1.0f;
    
    g_color_texture = texture::Create(&color_tex_desc, &color_clear_value);
    
    // Copy Cube data over
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *copy_list = copy_queue->GetCommandList();
    
    copy_list->CopyVertexBuffer(&g_vertex_buffer, _countof(g_cube_vertices), sizeof(VertexPosColor), g_cube_vertices);
    copy_list->CopyIndexBuffer(&g_index_buffer, _countof(g_cube_indicies), sizeof(u16), g_cube_indicies);
    
    g_diffuse_texture = copy_list->LoadTextureFromFile("textures/wall.jpg");
    assert(g_diffuse_texture.val != INVALID_TEXTURE_ID.val);
    
    copy_queue->ExecuteCommandLists(&copy_list, 1);
    
    Resource *diffuse_rsrc = texture::GetResource(g_diffuse_texture);
    g_diffuse_srv.Init(diffuse_rsrc);
    
    // Create Input Layout
    
    D3D12_DESCRIPTOR_RANGE1 texture_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    GfxInputElementDesc input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "COLOR",    0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    D3D12_ROOT_PARAMETER1 root_params[2];
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_params[0].Constants.ShaderRegister = 0;
    root_params[0].Constants.RegisterSpace = 0;
    root_params[0].Constants.Num32BitValues = sizeof(m4) / 4;
    
    // Diffuse texture
    root_params[1] = d3d::root_param1::InitAsDescriptorTable(1, &texture_range, D3D12_SHADER_VISIBILITY_PIXEL);
    
    // Diffuse texture sampler
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    g_root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(L"shaders/CubeVertex.cso");
    shader_modules.pixel  = LoadShaderModule(L"shaders/CubePixel.cso");
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature = &g_root_signature;
    pso_desc.shader_modules = shader_modules;
    pso_desc.blend = GetBlendState(BlendState::Disabled);
    pso_desc.depth  = GetDepthStencilState(DepthStencilState::ReadWrite);
    pso_desc.rasterizer = GetRasterizerState(RasterState::Default);
    pso_desc.input_layouts = input_desc;
    pso_desc.input_layouts_count = _countof(input_desc);
    pso_desc.topology = GfxTopology::Triangle;
    pso_desc.sample_desc.count = 1;
    pso_desc.rtv_formats = g_swapchain.GetRenderTarget()->GetRenderTargetFormats();
    pso_desc.dsv_format = GfxFormat::D32_Float;
    g_pipeline_state.Init(&pso_desc);
    
    copy_queue->Flush();
    
#endif
}

static void
RendererFree()
{
    device::Flush();
    
#if 0
    
    g_diffuse_srv.Free();
    texture::Free(g_diffuse_texture);
    
    g_root_signature.Free();
    g_vertex_buffer.Free();
    g_index_buffer.Free();
    g_pipeline_state.Free();
    
    texture::Free(g_color_texture);
    texture::Free(g_depth_texture);
    
#endif
    
    g_swapchain.Free();
    device::FreeDevice();
    FreeGlobalResourceState();
    
    device::ReportLiveObjects();
}

static void
RendererResize(u32 width, u32 height)
{
    device::Flush();
    // Resize the swap chain.
    g_swapchain.Resize(width, height);
}

static void 
RendererBeginFrame()
{
    CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);;
    g_frame_command_list = command_queue->GetCommandList();
    
    // Clear the current swapchain's render target
    RenderTarget *swap_rt = g_swapchain.GetRenderTarget();
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_frame_command_list->ClearTexture(swap_rt->GetTexture(AttachmentPoint::Color0), clear_color);
}

static void
RendererEndFrame()
{
    Assert(g_frame_command_list);
    CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);;
    
    command_queue->ExecuteCommandLists(&g_frame_command_list, 1);
    g_swapchain.Present();
    
    g_frame_command_list = 0;
}

static void 
RendererPresentFrame()
{
    g_swapchain.Present();
}

static CommandList*
RendererGetActiveCommandList()
{
    Assert(g_frame_command_list);
    return g_frame_command_list;
}

RENDERER_INTERFACE RenderError 
RendererEntry(RenderTarget *render_target, m4 view_proj_matrix)
{
    
#if 0
    
    static r32 s_delta_time = 0.0f; 
    static u64 s_frame_count = 0;
    static r32 s_total_time = 0;
    
    s_delta_time += 0.0006f;
    s_total_time += 0.0006f;
    s_frame_count += 1;
    
    if (s_total_time > 1.0f)
    {
        u64 fps = (u64)s_frame_count / (u64)s_total_time;
        s_frame_count = 0;
        s_total_time -= 1.0f;
        
        // TODO(Dustin): Log FPS?
    }
    
    //RenderTarget *render_target = g_swapchain.GetRenderTarget();
    //render_target->AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
    //RenderTarget *render_target = &g_render_target;
    render_target->AttachTexture(AttachmentPoint::Color0, g_color_texture);
    render_target->AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
    
    D3D12_VIEWPORT viewport = render_target->GetViewport();
    
    // Model Matrix
    r32 angle = (r32)s_delta_time * 90.0f;
    //r32 angle = (r32)90.0f;
    v3 rotation_axis = { 0, 1, 1 };
    m4 model_matrix = m4_rotate(angle, rotation_axis);
    
    // View matrix
    v3 eye_pos = { 0, 0, -10 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    m4 view_matrix = m4_look_at(eye_pos, look_at, up_dir);
    
    // Project matrix
    r32 aspect_ratio = (r32)viewport.Width / (r32)viewport.Height;
    m4 projection_matrix = m4_perspective(45.0f, aspect_ratio, 0.1f, 100.0f);
    
    // MVP Matrix
    //m4 mvp = m4_mul(projection_matrix, view_matrix);
    m4 mvp = m4_mul(view_proj_matrix, model_matrix);
    
    CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);;
    CommandList *command_list = command_queue->GetCommandList();
    
    // Set the graphics pipeline
    command_list->SetPipelineState(g_pipeline_state._handle);
    command_list->SetGraphicsRootSignature(&g_root_signature);
    
    // Set root parameters
    command_list->SetGraphics32BitConstants(0, &mvp);
    
    command_list->SetRenderTarget(render_target);
    
    // TODO(Dustin): Need to implement ClearTexture(TEXTURE_ID)
    r32 clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearTexture(render_target->GetTexture(AttachmentPoint::Color0), clear_color);
    command_list->ClearDepthStencilTexture(render_target->GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    
    command_list->SetViewport(viewport);
    
    D3D12_RECT scissor_rect = {};
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = (LONG)viewport.Width;
    scissor_rect.bottom = (LONG)viewport.Height;
    command_list->SetScissorRect(scissor_rect);
    
    // Render the cube
    
    command_list->SetShaderResourceView(1, 0, &g_diffuse_srv, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    command_list->SetVertexBuffer(0, &g_vertex_buffer);
    command_list->SetIndexBuffer(&g_index_buffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->DrawIndexedInstanced((UINT)g_index_buffer._count);
    
    // NOTE(Dustin): This is not ideal...
    // Clear the primary swapchain image
    
    command_queue->ExecuteCommandLists(&command_list, 1);
    
#endif
    
    return RenderError::Success;
}

static void
RendererClearRootSwapchain(CommandList *command_list)
{
    RenderTarget *swap_rt = g_swapchain.GetRenderTarget();
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list->ClearTexture(swap_rt->GetTexture(AttachmentPoint::Color0), clear_color);
}
