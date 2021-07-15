#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

//~ Header

#if 0
// TODO(Matt): Change "automatic_rebuild" to be "manual_rebuild". We'd like the default (0 or false)
// value to be the most common use case. Most of the time automatic rebuild should be enabled.
struct SwapChainConfig
{
    // Total number of buffers. Must be between 2 and DXGI_MAX_SWAP_CHAIN_BUFFERS.
    u32 buffer_count;
    
    // Max number of queued frames. Must be at least 1.
    u32 max_frame_latency;
    
    // VSync interval, must be between 0 and 4. A value of 0 disables VSync, and 1 enables it.
    // Set to 2, 3, or 4 to sync at 1/2, 1/3, or 1/4 the refresh rate.
    u32 sync_interval;
    
    // If true, swap chain will detect window size, buffer count, or max frame latency changes, and
    // resize automatically in BeginFrame(). If false, you must rebuild manually.
    bool automatic_rebuild;
};
#endif

struct Swapchain
{
    // Current config. NOTE(Matt): If using automatic rebuild, changing the config can force a
    // resize or rebuild during the next BeginFrame() call, incurring a GPU flush.
    // Changing the buffer count will cause a resize, and changing the max frame latency will force
    // a full rebuild.
    //SwapChainConfig config;
    
    struct CommandQueue *_present_queue;
    
    // We use an IDXGISwapChain3 because we need to be able to call GetCurrentBackBufferIndex().
    IDXGISwapChain3*     _handle;
    
    // Index of the current back buffer. This gets updated during every call to BeginFrame().
    u32                  _frame_index;
    // Per-buffer resources, including the render target and last fence signal value for each back
    // buffer. To check if a back buffer is in use, you can compare its fence value to the "last
    // completed value" of the fence.
    u64                  _fence_values[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    DXGI_FORMAT          _render_target_format;
    RenderTarget         _render_target;
    TEXTURE_ID           _back_buffer_textures[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    // current width/height of the swapchain
    u32                  _width;
    u32                  _height;
    // true if the swapchain is currently fullscreen
    bool                 _fullscreen;
    // true if tearing is supported
    bool                 _tearing_supported;
    bool                 _vsync;
    // Waitable handles. We use a waitable swap chain in order to limit the number of queued frames,
    // and we can wait on these handles to ensure that we don't use or free resourcese that are in
    // use by the GPU, and to defer updating/rendering until we are sure we will have a buffer to
    // render into.
    HANDLE               _buffer_available_event;
    
    /*
    Initializes the swap chain, tying it to a platform window. It's usually a good idea to enable
    automatic rebuild. If you're unsure, a buffer count of 3 and maximum frame latency of 2 is a
    reasonable config for most applications.
    */
    void Init(HWND hwnd, CommandQueue *present_queue, u32 width, u32 height);
    void Free();
    
    /*
    Call at the beginning of every frame, ideally before game update or input handling.
    If wait_for_buffer is true, this will block until we have a buffer available to render into.
    This reduces latency in cases where the present queue is full (GPU bound). You can ignore the
    return value in this case (it will always be true).
    
    If wait_for_buffer is false, this function will not block, and the return value indicates
     whether there is an available buffer or not. A common use for this is to render into an
    off-screen buffer, and only present if there is a swapchain buffer available. This gives
    a properly unlocked framerate (for benchmarks or whatever), but offers no reduction in display
    latency, and the computer will be going at full speed for no reason.
    */
    //void PrepareFrame(bool wait_for_buffer = true);
    
    
    /*
    Presents the current frame, increments the current fence value, and signals the command queue
    with that new value. The new value is also cached in the array of per-buffer fence values.
    If configured with sync interval 0, VSync is off, and the "allow tearing" flag will be used
    if available. Otherwise, the "allow tearing" flag is unused, and the configured sync interval
    is used for VSync. Note that for most typical configs, if you are allowing BeginFrame() to
    block, the call to Present() shouldn't have to.
    */
    //void PresentFrame();
    
    /*
    These two methods get the back buffer and RTV handle for the current frame. Mostly useful for
    command list recording, such as inserting a transition barrier or drawing to the back buffer.
    It is only valid to call these somewhere between BeginFrame() and PresentFrame(). 
    */
    //ID3D12Resource* GetCurrentBackBuffer();
    //D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTVHandle();
    
    /*
    Resize the swap chain. If automatic rebuild is enabled, you likely don't need to call this
    manually, as it will be called during BeginFrame() if needed. If not, you'll want to call
    this in response to window size or buffer count changes. If there are any buffers still
    in-flight, this will block until they are no longer in use.
    */
    void Resize(u32 width, u32 height);
    
    /*
    Fully rebuild the swap chain. Just like Resize(), you likely don't need to call this if
    automatic rebuild is enabled. It will be called during BeginFrame() if needed. Otherwise,
    you should only have to call this if you change the maximum frame latency. Other config changes
    should be handled by calling Resize(). If there are any buffers still in-flight, this will
    block until they are no longer in use.
    */
    //void Rebuild();
    
    /**
         * Present the swapchain's back buffer to the screen.
         *
         * @param [texture] The texture to copy to the swap chain's backbuffer before
         * presenting. By default, this is an empty texture. In this case, no copy
         * will be performed. Use the SwapChain::GetRenderTarget method to get a render
         * target for the window's color buffer.
         *
         * @returns The current backbuffer index after the present.
         */
    u32 Present(TEXTURE_ID tex_id = INVALID_TEXTURE_ID);
    
    /**
         * Get the render target of the window. This method should be called every
         * frame since the color attachment point changes depending on the window's
         * current back buffer.
         */
    RenderTarget* GetRenderTarget();
    
    void UpdateRenderTargetViews();
};


#endif //_SWAPCHAIN_H
