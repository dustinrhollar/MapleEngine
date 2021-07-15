
struct VertexBuffer
{
    //ID3D12Resource          *_handle;
    Resource                 _resource;
    D3D12_VERTEX_BUFFER_VIEW _buffer_view;
    u64                      _count;
    u64                      _stride;
    
    RenderError Init(ID3D12Resource *resource, u64 vertex_count, u64 stride);
    RenderError Free();
    
    FORCE_INLINE ID3D12Resource* 
        GetResource()
    {
        return _resource._handle;
    }
};

struct IndexBuffer
{
    //ID3D12Resource          *_handle;
    Resource                _resource;
    D3D12_INDEX_BUFFER_VIEW _buffer_view;
    size_t                  _count;
    DXGI_FORMAT             _format;
    
    RenderError Init(ID3D12Resource *resource, u64 indices_count, u64 stride);
    RenderError Free();
    
    FORCE_INLINE ID3D12Resource* 
        GetResource()
    {
        return _resource._handle;
    }
};



RenderError 
VertexBuffer::Init(ID3D12Resource *resource, u64 vertex_count, u64 stride)
{
    RenderError result = RenderError::Success;
    
    _resource.Init(resource);
    _count = vertex_count;
    _stride = stride;
    
    _buffer_view = {};
    _buffer_view.BufferLocation = _resource._handle->GetGPUVirtualAddress();
    _buffer_view.StrideInBytes  = (UINT)stride;
    _buffer_view.SizeInBytes    = (UINT)(vertex_count * stride);
    
    return result;
}

RenderError 
VertexBuffer::Free()
{
    RenderError result = RenderError::Success;
    _resource.Free();
    return result;
}


RenderError 
IndexBuffer::Init(ID3D12Resource *resource, u64 indices_count, u64 stride)
{
    assert(stride == sizeof(u16) || stride == sizeof(u32));
    RenderError result = RenderError::Success;
    
    _resource.Init(resource);
    _count = indices_count;
    _format = (stride == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    
    _buffer_view = {};
    _buffer_view.BufferLocation = _resource._handle->GetGPUVirtualAddress();
    _buffer_view.Format  = _format;
    _buffer_view.SizeInBytes = (UINT)(indices_count * stride);
    
    return result;
}

RenderError IndexBuffer::Free()
{
    RenderError result = RenderError::Success;
    _resource.Free();
    return result;
}
