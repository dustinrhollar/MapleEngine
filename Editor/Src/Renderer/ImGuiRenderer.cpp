
namespace d3d_imgui
{
    RootSignature       g_root_signature{};
    PipelineStateObject g_pipeline_state{};
    TEXTURE_ID          g_font_texture;
    //ShaderResourceView  g_font_srv;
    
    // Root parameters for the ImGui root signature.
    enum RootParameters
    {
        MatrixCB,     // cbuffer vertexBuffer : register(b0)
        FontTexture,  // Texture2D texture0 : register(t0);
        NumRootParameters
    };
    
    static void Initialize(struct RenderTarget *render_target);
    static void Free();
    static void Render(ImDrawData* draw_data, struct RenderTarget *render_target);
    
    // Viewport Implementation
    
    struct ViewportData
    {
        Swapchain _swapchain;
    };
    
    static void ViewportCreateWindow(ImGuiViewport *viewport);
    static void ViewportDestroyWindow(ImGuiViewport* viewport);
    static void ViewportSetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    static void ViewportRenderWindow(ImGuiViewport* viewport, void*);
    static void ViewportSwapBuffers(ImGuiViewport* viewport, void*);
    static void InitPlatformInterface();
    static void ShutdownPlatformInterface();
    
};

struct VERTEX_CONSTANT_BUFFER
{
    float mvp[4][4];
};

static void 
d3d_imgui::Initialize(RenderTarget *render_target)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx12";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional) // FIXME-VIEWPORT: Actually unfinished..
    
    // Create a dummy ImGuiViewportDataDx12 holder for the main viewport,
    // Since this is created and managed by the application, we will only use the ->Resources[] fields.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->RendererUserData = IM_NEW(ViewportData)();
    
    // Setup backend capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can create multi-viewports on the Renderer side (optional)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        InitPlatformInterface();
    
    CommandQueue *copy_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *cmd_list = copy_queue->GetCommandList();
    
    // Create fonts texture
    {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        
        g_font_texture = cmd_list->LoadTextureFromMemory(pixels, width, height, 4, false);
        //g_font_srv.Init(texture::GetResource(g_font_texture));
        
        Texture *texture = texture::GetTexture(g_font_texture);
        
        copy_queue->ExecuteCommandLists(&cmd_list, 1);
        
        io.Fonts->SetTexID((ImTextureID)((uptr)g_font_texture.val));
    }
    
    // Create Root Signature
    {
        D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        
        D3D12_DESCRIPTOR_RANGE1 texture_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        
        D3D12_ROOT_PARAMETER1 root_params[2];
        root_params[RootParameters::MatrixCB] = d3d::root_param1::InitAsConstant(16, 0, 0,
                                                                                 D3D12_SHADER_VISIBILITY_VERTEX);
        root_params[RootParameters::FontTexture] = d3d::root_param1::InitAsDescriptorTable(1, &texture_range, 
                                                                                           D3D12_SHADER_VISIBILITY_PIXEL);
        
        // Diffuse texture sampler
        D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
        linear_sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        linear_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        g_root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
        
    }
    
    // Create Pipeline State Object
    {
        GfxShaderModules shader_modules;
        shader_modules.vertex = LoadShaderModule(L"shaders/ImGuiVertex.cso");
        shader_modules.pixel  = LoadShaderModule(L"shaders/ImGuiPixel.cso");
        
        GfxInputElementDesc input_desc[] = {
            { "POSITION", 0, GfxFormat::R32G32_Float,   0, offsetof(ImDrawVert, pos), GfxInputClass::PerVertex, 0 },
            { "TEXCOORD", 0, GfxFormat::R32G32_Float,   0, offsetof(ImDrawVert, uv),  GfxInputClass::PerVertex, 0 },
            { "COLOR",    0, GfxFormat::R8G8B8A8_Unorm, 0, offsetof(ImDrawVert, col), GfxInputClass::PerVertex, 0 },
        };
        
        GfxPipelineStateDesc pso_desc{};
        pso_desc.root_signature = &g_root_signature;
        pso_desc.shader_modules = shader_modules;
        pso_desc.input_layouts = input_desc;
        pso_desc.input_layouts_count = _countof(input_desc);
        pso_desc.topology = GfxTopology::Triangle;
        pso_desc.blend = GetBlendState(BlendState::Traditional);
        pso_desc.depth  = GetDepthStencilState(DepthStencilState::Disabled);
        pso_desc.rasterizer = GetRasterizerState(RasterState::DefaultCw);
        pso_desc.rtv_formats = render_target->GetRenderTargetFormats();
        pso_desc.sample_desc.count = 1;
        g_pipeline_state.Init(&pso_desc);
    }
}

static void
d3d_imgui::Free()
{
    // Manually delete main viewport render resources in-case we haven't initialized for viewports
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (ViewportData* data = (ViewportData*)main_viewport->RendererUserData)
    {
        IM_DELETE(data);
        main_viewport->RendererUserData = NULL;
    }
    
    ShutdownPlatformInterface();
    
    //g_font_srv.Free();
    texture::Free(g_font_texture);
    g_pipeline_state.Free();
    g_root_signature.Free();
}

static void
d3d_imgui::Render(ImDrawData* draw_data, RenderTarget *render_target)
{
    CommandList *command_list = RendererGetActiveCommandList();
    ImGuiIO&    io       = ImGui::GetIO();
    //ImDrawData* draw_data = ImGui::GetDrawData();
    
    // Check if there is anything to render.
    if ( !draw_data || draw_data->CmdListsCount == 0 )
        return;
    
    ImVec2 displayPos = draw_data->DisplayPos;
    
    command_list->SetPipelineState(g_pipeline_state._handle);
    command_list->SetGraphicsRootSignature(&g_root_signature);
    command_list->SetRenderTarget(render_target);
    
    // Set root arguments.
    //    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixOrthographicRH( draw_data->DisplaySize.x,
    //    draw_data->DisplaySize.y, 0.0f, 1.0f );
    float L         = draw_data->DisplayPos.x;
    float R         = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T         = draw_data->DisplayPos.y;
    float B         = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float mvp[4][4] = {
        { 2.0f / ( R - L ), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / ( T - B ), 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.5f, 0.0f },
        { ( R + L ) / ( L - R ), ( T + B ) / ( B - T ), 0.5f, 1.0f },
    };
    
    command_list->SetGraphics32BitConstants(RootParameters::MatrixCB, 16, mvp);
    
    D3D12_VIEWPORT viewport = {};
    viewport.Width          = draw_data->DisplaySize.x;
    viewport.Height         = draw_data->DisplaySize.y;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    
    command_list->SetViewport(viewport);
    command_list->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    const DXGI_FORMAT indexFormat = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    
    // It may happen that ImGui doesn't actually render anything. In this case,
    // any pending resource barriers in the command_list will not be flushed (since
    // resource barriers are only flushed when a draw command is executed).
    // In that case, manually flushing the resource barriers will ensure that
    // they are properly flushed before exiting this function.
    command_list->FlushResourceBarriers();
    
    for ( int i = 0; i < draw_data->CmdListsCount; ++i )
    {
        const ImDrawList* drawList = draw_data->CmdLists[i];
        
        command_list->SetDynamicVertexBuffer(0, (u64)drawList->VtxBuffer.size(), (u64)sizeof(ImDrawVert),
                                             (void*)drawList->VtxBuffer.Data );
        command_list->SetDynamicIndexBuffer((u64)drawList->IdxBuffer.size(), indexFormat, (void*)drawList->IdxBuffer.Data);
        
        int indexOffset = 0;
        for ( int j = 0; j < drawList->CmdBuffer.size(); ++j )
        {
            const ImDrawCmd& drawCmd = drawList->CmdBuffer[j];
            if ( drawCmd.UserCallback )
            {
                drawCmd.UserCallback( drawList, &drawCmd );
            }
            else
            {
                ImVec4     clipRect = drawCmd.ClipRect;
                D3D12_RECT scissorRect;
                scissorRect.left   = static_cast<LONG>( clipRect.x - displayPos.x );
                scissorRect.top    = static_cast<LONG>( clipRect.y - displayPos.y );
                scissorRect.right  = static_cast<LONG>( clipRect.z - displayPos.x );
                scissorRect.bottom = static_cast<LONG>( clipRect.w - displayPos.y );
                
                if ( scissorRect.right - scissorRect.left > 0.0f && scissorRect.bottom - scissorRect.top > 0.0 )
                {
                    TEXTURE_ID texture_id;
                    texture_id.val = (u32)((uptr)drawCmd.GetTexID());
                    
                    command_list->SetShaderResourceView(RootParameters::FontTexture, 0, texture_id,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    command_list->SetScissorRect(scissorRect);
                    command_list->DrawIndexedInstanced(drawCmd.ElemCount, 1, indexOffset);
                }
            }
            indexOffset += drawCmd.ElemCount;
        }
    }
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void 
d3d_imgui::ViewportCreateWindow(ImGuiViewport *viewport)
{
    ViewportData *data = IM_NEW(ViewportData)();
    viewport->RendererUserData = data;
    data->_swapchain.Init(viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle, 
                          device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT), 
                          (u32)viewport->Size.x, (u32)viewport->Size.y);
}

static void 
d3d_imgui::ViewportDestroyWindow(ImGuiViewport* viewport)
{
    if (ViewportData* data = (ViewportData*)viewport->RendererUserData)
    {
        CommandQueue *command_queue = data->_swapchain._present_queue;
        command_queue->Flush();
        data->_swapchain.Free();
        IM_DELETE(data);
    }
    viewport->RendererUserData = NULL;
}

static void 
d3d_imgui::ViewportSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ViewportData* data = (ViewportData*)viewport->RendererUserData;
    
    //CommandQueue *command_queue = data->_swapchain._present_queue;
    device::Flush();
    
    data->_swapchain.Resize((u32)size.x, (u32)size.y);
}

static void
d3d_imgui::ViewportRenderWindow(ImGuiViewport* viewport, void*)
{
    ViewportData* data = (ViewportData*)viewport->RendererUserData;
    
    //CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    //CommandList *command_list = command_queue->GetCommandList();
    CommandList *command_list = RendererGetActiveCommandList();
    
    ImGuiIO&    io       = ImGui::GetIO();
    ImDrawData* draw_data = ImGui::GetDrawData();
    
    RenderTarget *render_target = data->_swapchain.GetRenderTarget();
    
    r32 clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    command_list->ClearTexture(render_target->GetTexture(AttachmentPoint::Color0), clear_color);
    
    Render(viewport->DrawData, render_target);
    
    //command_queue->ExecuteCommandLists(&command_list, 1);
}

static void 
d3d_imgui::ViewportSwapBuffers(ImGuiViewport* viewport, void*)
{
    ViewportData* data = (ViewportData*)viewport->RendererUserData;
    data->_swapchain.Present();
}

static void 
d3d_imgui::InitPlatformInterface()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ViewportCreateWindow;
    platform_io.Renderer_DestroyWindow = ViewportDestroyWindow;
    platform_io.Renderer_SetWindowSize = ViewportSetWindowSize;
    platform_io.Renderer_RenderWindow = ViewportRenderWindow;
    platform_io.Renderer_SwapBuffers = ViewportSwapBuffers;
}

static void 
d3d_imgui::ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}
