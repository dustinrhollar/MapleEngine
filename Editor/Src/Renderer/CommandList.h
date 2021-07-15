#ifndef _COMMAND_LIST_H
#define _COMMAND_LIST_H

struct CommandList
{
    RenderError Init(D3D12_COMMAND_LIST_TYPE list_type);
    RenderError Free();
    
    /* Resets the Command List and Allocator and begins recording */
    
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
    void UAVBarrier(ID3D12Resource* resource, bool flushBarriers = false);
    void AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flushBarriers = false );
    
    void FlushResourceBarriers();
    
    void CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes);
    void CopyResource(struct Resource* dstRes, struct Resource* srcRes);
    void ResolveSubresource(Resource* dst_resource, Resource* src_resouce,
                            u32 dstSubresource = 0, u32 srcSubresource = 0 );
    
    // TODO(Dustin): Resolve subresource (for mipmapping)
    
    void CopyVertexBuffer(struct VertexBuffer* vertexBuffer, u64 numVertices, u64 vertexStride, void* vertexBufferData);
    template<typename T>
        void CopyVertexBuffer(struct VertexBuffer* vertexBuffer, T* vertexBufferData, u64 count)
    {
        CopyVertexBuffer(vertexBuffer, count, sizeof(T), vertexBufferData);
    }
    
    void CopyIndexBuffer(struct IndexBuffer* indexBuffer, u64 numIndices, u64 stride, void* index_data);
    template<typename T>
        void CopyIndexBuffer(struct IndexBuffer* indexBuffer, T* indexBufferData, u64 count)
    {
        assert(sizeof(T) == 2 || sizeof(T) == 4);
        //DXGI_FORMAT format = ( sizeof(T) == 2 ) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        CopyIndexBuffer(indexBuffer, count, sizeof(T), vertexBufferData);
    }
    
    // TODO(Dustin): Constant Buffer, Byte Address Buffer, Structured Buffer
    
    void SetScissorRect(const D3D12_RECT &scissor_rect);
    void SetScissorRects(const D3D12_RECT *scissor_rect, u32 num_rects);
    
    void SetViewport(const D3D12_VIEWPORT &viewport);
    void SetViewports(const D3D12_VIEWPORT *viewports, u32 num_viewports);
    
    void ClearTexture(TEXTURE_ID tex_id, r32 clear_color[4]);
    void ClearDepthStencilTexture(TEXTURE_ID tex_id, D3D12_CLEAR_FLAGS clearFlags,
                                  r32 depth = 1.0f, u8 stencil = 0 );
    
    void SetDynamicVertexBuffer(u32 slot, u64 numVertices, u64 vertexSize, void* vertexBufferData );
    template<typename T>
        void SetDynamicVertexBuffer(u32 slot, u64 numVertices, T* vertexBufferData)
    {
        SetDynamicVertexBuffer(slot, numVertices, sizeof(T), vertexBufferData);
    }
    
    void SetDynamicIndexBuffer(u64 numIndices, DXGI_FORMAT format, void* indexBufferData );
    template<typename T>
        void SetDynamicIndexBuffer(u32 slot, u64 numIndices, T* indexBufferData)
    {
        assert(sizeof(T) == 2 || sizeof(T) == 4);
        DXGI_FORMAT format = ( sizeof(T) == 2 ) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        SetDynamicIndexBuffer(slot, numIndices, format, IndexBufferData);
    }
    
    void SetGraphics32BitConstants(u32 rootParameterIndex, u32 numConstants, void* constants);
    template<typename T>
        void SetGraphics32BitConstants(u32 rootParameterIndex, T* constants)
    {
        static_assert(sizeof(T) % sizeof(u32) == 0, "Size of type must be a multiple of 4 bytes" );
        SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(u32), (void*)constants);
    }
    
    // Set a set of 32-bit constants on the compute pipeline.
    void SetCompute32BitConstants(u32 rootParameterIndex, u32 numConstants, const void* constants);
    template<typename T>
        void SetCompute32BitConstants(u32 rootParameterIndex, const T& constants)
    {
        static_assert( sizeof( T ) % sizeof( u32 ) == 0, "Size of type must be a multiple of 4 bytes" );
        SetCompute32BitConstants(rootParameterIndex, sizeof( T ) / sizeof( u32 ), &constants);
    }
    
    void SetGraphicsRootSignature(struct RootSignature *root_sig);
    void SetComputeRootSignature(struct RootSignature *root_sig);
    
    void SetPipelineState(ID3D12PipelineState *pipeline_state);
    void SetTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
    
    // NOTE(Dustin): Internal heap allocation. See about removing...
    void SetVertexBuffers(u32 startSlot, struct VertexBuffer** vertexBuffers, u32 buf_count);
    void SetVertexBuffer(u32 slot, struct VertexBuffer* vertex_buffer);
    void SetIndexBuffer(struct IndexBuffer* indexBuffer);
    
    void Dispatch(u32 numGroupsX, u32 numGroupsY = 1, u32 numGroupsZ = 1);
    void DrawIndexedInstanced(UINT index_count_per_instance,
                              UINT instance_count = 1,
                              UINT start_index_location = 0,
                              INT  base_vertex_location = 0,
                              UINT start_instance_location = 0);
    void DrawInstanced(u32 vtx_count_per_instance,
                       u32 instance_count          = 1,
                       u32 start_vtx_location      = 0,
                       u32 start_instance_location = 0);
    
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
    void BindDescriptorHeaps();
    
    void TrackObject(ID3D12Object* object);
    void TrackResource(ID3D12Resource *resource);
    void ReleaseTrackedObjects();
    
    bool ClosePending(CommandList *pending_cmd_list);
    void Close();
    
    // Resets the command list. Should only be called by the command
    // queue when the command list is returned from GetCommandList
    void Reset();
    
    void SetRenderTarget(struct RenderTarget *render_target);
    
    void CopyTextureSubresource(TEXTURE_ID tex_id, u32 first_subresource, u32 num_subresources,
                                D3D12_SUBRESOURCE_DATA *subresources);
    
    
    TEXTURE_ID LoadTextureFromFile(const char *filename, bool gen_mips = true);
    TEXTURE_ID LoadTextureFromMemory(void *pixels, int width, int height, int num_channels, bool gen_mips = true);
    void GenerateMips(TEXTURE_ID tex_id);
    void GenerateMips_UAV(TEXTURE_ID tex_id, bool is_srgb);
    
    // Set an inline SRV.
    // Note: Only Buffer resources can be used with inline SRV's
    void SetShaderResourceView(u32  root_param_idx, Resource *buffer,
                               D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               u64 buffer_offset = 0 );
    
    // Set the SRV on the graphics pipeline.
    void SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset,
                               struct ShaderResourceView *srv,
                               D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               UINT firstSubresource = 0,
                               UINT numSubresources  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
    
    // Set an SRV on the graphics pipeline using the default SRV for the texture.
    void SetShaderResourceView(i32 rootParameterIndex, u32 descriptorOffset,
                               TEXTURE_ID texture,
                               D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               UINT firstSubresource = 0,
                               UINT numSubresources  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
    
    // Set an inline CBV.
    // Note: Only Buffer resoruces can be used with inline UAV's.
    void SetConstantBufferView(u32 root_param_idx, Resource *buffer,
                               D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                               u64 buffer_offset = 0);
    
    // Set the CBV on the rendering pipeline.
    void SetConstantBufferView(u32 root_param_idx, u32 descriptorOffset,
                               struct ConstantBufferView *cbv,
                               D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
    
    //Set an inline UAV.
    //Note: Only Buffer resoruces can be used with inline UAV's.
    void SetUnorderedAccessView(u32 root_param_idx, Resource *buffer,
                                D3D12_RESOURCE_STATES stateAfter   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                u64 buffer_offset = 0);
    
    // Set the UAV on the graphics pipeline.
    void SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
                                struct UnorderedAccessView *uav,
                                D3D12_RESOURCE_STATES stateAfter       = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                UINT                  firstSubresource = 0,
                                UINT                  numSubresources  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
    
    // Set the UAV on the graphics pipline using a specific mip of the texture.
    void SetUnorderedAccessView(u32 rootParameterIndex, u32 descriptorOffset,
                                TEXTURE_ID texture, UINT mip,
                                D3D12_RESOURCE_STATES stateAfter       = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                UINT                  firstSubresource = 0,
                                UINT                  numSubresources  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
    
    //------------------------------------------------------------------
    // @INTERNAL
    
    ID3D12Resource* CopyBuffer(u64 bufferSize, const void* bufferData,
                               D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    
    using TrackedObjects = ID3D12Object**; // stb array of ID3D12Object*
    
    ID3D12GraphicsCommandList *_handle;
    ID3D12CommandAllocator    *_allocator;
    
    D3D12_COMMAND_LIST_TYPE    _type;
    // Currently bound root signature
    ID3D12RootSignature       *_root_signature = 0;
    ID3D12PipelineState       *_pipeline_state = 0;
    // Resource created in an upload heap
    // Useful for drawing dynamic geometry or for 
    // uploading constant ubvffer data that changes every draw
    UploadBuffer               _upload_buffer;
    // Resource State tracker is used to track (per command list) the current
    // state of a resource. Will also track the global state of a resource in 
    // order to reduce state transition
    ResourceStateTracker       _resource_state_tracker;
    
    // TODO(Dustin): Add back in
    // Dynamic descriptor heap allos for descriptors to be staged before being
    // committed to the command list. Dynamic descriptors need to be committed
    // before a draw or dispatch
    DynamicDescriptorHeap      _dynamic_descriptor_heap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    // Keep track of the currently bound descriptor heaps. Only change heaps
    // if they are different than the current bound heap.
    ID3D12DescriptorHeap*      _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    
    // Objects that beig tracked by a command list that is "in-flight" on the command
    // queue and cannot be deleted. To ensure objects are not deleted until the command
    // list is finished executing, a refernence to the object is stored. The refernenced 
    // objects are released when the command list is reset
    TrackedObjects             _tracked_objects = 0;
    
    // Mip generation
    
    struct alignas(16) GenerateMipsCB
    {
        u32 SrcMipLevel;   // Texture level of source mip
        u32 NumMipLevels;  // Number of OutMips to write: [1-4]
        u32 SrcDimension;  // Width and height of the source texture are even or odd.
        u32 IsSRGB;        // Must apply gamma correction to sRGB textures.
        v2  TexelSize;     // 1.0 / OutMip1.Dimensions
    };
    
    enum class GenerateMips
    {
        GenerateMipsCB,
        SrcMip,
        OutMip,
        NumRootParameters
    };
    
    struct GenerateMipsPSO
    {
        void Init();
        void Free();
        
        D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV()
        {
            return _default_uav.GetDescriptorHandle();
        }
        
        RootSignature        _root_signature;
        PipelineStateObject  _pipeline_state;
        // Default (no resource) UAV's to pad the unused UAV descriptors.
        // If generating less than 4 mip map levels, the unused mip maps
        // need to be padded with default UAVs (to keep the DX12 runtime happy).
        DescriptorAllocation _default_uav;
    } _generate_mips_pso = {};
    
    // MIPS command list
    CommandList *_compute_command_list = 0;
    
    
    
};

#endif //_COMMAND_LIST_H
