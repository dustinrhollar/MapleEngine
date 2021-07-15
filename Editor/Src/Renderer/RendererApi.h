#ifndef _RENDERER_API_H
#define _RENDERER_API_H

#ifdef _RENDERER_EXPORT
#define RENDERER_INTERFACE extern "C" _declspec(dllexport)
#else
//#define RENDERER_INTERFACE extern "C" _declspec(dllimport)
#define RENDERER_INTERFACE
#endif

enum class RenderError : u8
{
    Success,
    DebugDeviceUnavailable,
    TearingUnavailable,
    DXGI_1_5_NotSupported,
    DXGI_1_6_NotSupported,
    BadAdapter,
    WARPNotAvailable,
    BadDevice,
    BadCommandQueue,
    BadDescriptorHeap,
    BadCommandAllocator,
    BadCommandList,
    CommandListCloseError,
    BadFence,
    BadEvent,
    BadRenderTargetView,
    SwapchainPresentError,
    FenceSignalError,
    EventSetError,
    
    BadSwapchain,       // Failed to initialize or get the swapchain
    BadSwapchainBuffer, // Failed to acquire backbuffer for a swapchain
    SwapchainNotReady,  // Set when fence for swapchain is not ready for presentation.
    
    CommandAllocatorResetError,
    CommandListResetError,
    
    BufferResourceCreateError,
    
    RootSignatureError,
    
    ResourceMapError,
    
    ShaderBlobError,
    
    PipelineStateError,
    
    Count,
    Unknown = Count,
};

struct RendererCallbacks
{
};

struct RendererInitInfo
{
    void                 *wnd;
    u32                   wnd_width;
    u32                   wnd_height;
    RendererCallbacks     callbacks;
};

typedef void (*PFN_RendererInit)(RendererInitInfo *info);
typedef void (*PFN_RendererFree)();
typedef void (*PFN_RendererEntry)();

#ifndef _RENDERER_NO_PROTOTYPES

static void RendererInit(RendererInitInfo *info);
static void RendererFree();
static void RendererBeginFrame();
static void RendererEndFrame();
static struct CommandList* RendererGetActiveCommandList();

//RENDERER_INTERFACE RenderError RendererInit(RendererInitInfo *info);
//RENDERER_INTERFACE RenderError RendererFree();
//RENDERER_INTERFACE RenderError RendererEntry(/* TODO(Dustin): CommandList(s) */);
static void RendererPresentFrame();

#endif

#endif //_RENDERER_H
