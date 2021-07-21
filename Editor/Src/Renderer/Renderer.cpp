
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

// TODO(Dustin): Get rid of this!
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

#include "Geometry/Common.h"
#include "Geometry/Sphere.cpp"
#include "Geometry/Cube.cpp"

static Swapchain           g_swapchain{};      // @INTERNAL
// TODO(Dustin): When moving to a multithreaded renderer,
// should set a single command list for each thread
static CommandList *g_frame_command_list = 0;

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
}

static void
RendererFree()
{
    device::Flush();
    
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
    if (!g_frame_command_list)
    {
        CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);;
        g_frame_command_list = command_queue->GetCommandList();
    }
    
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
    if (!g_frame_command_list)
    {
        g_frame_command_list = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetCommandList();
    }
    return g_frame_command_list;
}

RENDERER_INTERFACE RenderError 
RendererEntry(RenderTarget *render_target, m4 view_proj_matrix)
{
    return RenderError::Success;
}

static void
RendererClearRootSwapchain(CommandList *command_list)
{
    RenderTarget *swap_rt = g_swapchain.GetRenderTarget();
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list->ClearTexture(swap_rt->GetTexture(AttachmentPoint::Color0), clear_color);
}
