
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

struct ByteAddressBuffer
{
    void Init(D3D12_RESOURCE_DESC *desc);
    void Init(ID3D12Resource *resource);
    void Free();
    
    Resource _resource;
    u64      _size;
};

struct ConstantBuffer
{
    void Init(ID3D12Resource *resource);
    void Free();
    
    Resource _resource;
    u64      _size;
};

struct StructuredBuffer
{
    void Init(u64 element_count, u64 stride);
    void Init(ID3D12Resource *resource, u64 element_count, u64 stride);
    void Free();
    
    Resource _resource;
    u64               _num_elements;
    u64               _stride;
    // A buffer to store the internal counter for the structured buffer
    ByteAddressBuffer _counter_buffer;
};

struct ConstantBufferView
{
    void Init(ConstantBuffer *cb, u64 offset = 0)
    {
        ID3D12Device *d3d_device = device::GetDevice();
        ID3D12Resource *rsrc = cb->_resource._handle;
        
        _cb = cb;
        
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
        cbv.BufferLocation = rsrc->GetGPUVirtualAddress() + offset;
        cbv.SizeInBytes = memory_align(_cb->_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        
        _descriptor = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        d3d_device->CreateConstantBufferView(&cbv, _descriptor.GetDescriptorHandle());
    }
    
    void Free()
    {
        _cb = 0;
        _descriptor.Free();
        _descriptor = {};
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return _descriptor.GetDescriptorHandle();
    }
    
    ConstantBuffer      *_cb;
    DescriptorAllocation _descriptor;
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
    //_resource.Free();
    
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
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

RenderError
IndexBuffer::Free()
{
    RenderError result = RenderError::Success;
    //_resource.Free();
    
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
    return result;
}

void 
ByteAddressBuffer::Init(D3D12_RESOURCE_DESC *desc)
{
    _size = desc->Width;
    _resource.Init(desc);
}

void 
ByteAddressBuffer::Init(ID3D12Resource *resource)
{
    _resource.Init(resource);
    _size = _resource.GetResourceDesc().Width;
}

void 
ByteAddressBuffer::Free()
{
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
    //_resource.Free();
    _size = 0;
}

void
ConstantBuffer::Init(ID3D12Resource *resource)
{
    _resource.Init(resource);
    _size = _resource.GetResourceDesc().Width;
}

void
ConstantBuffer::Free()
{
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
    //_resource.Free();
}

void
StructuredBuffer::Init(u64 element_count, u64 stride)
{
    _num_elements = element_count;
    _stride = stride;
    
    // TODO(Dustin): Create buffer desc
    D3D12_RESOURCE_DESC desc = d3d::GetBufferResourceDesc(_num_elements * _stride, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    _resource.Init(&desc);
    
    // TODO(Dustin): Create a byte address buffer (4)
    desc = d3d::GetBufferResourceDesc(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    _counter_buffer.Init(&desc);
}

void
StructuredBuffer::Init(ID3D12Resource *resource, u64 element_count, u64 stride)
{
    _num_elements = element_count;
    _stride = stride;
    _resource.Init(resource);
    
    // TODO(Dustin): Create a byte address buffer  (4)
    D3D12_RESOURCE_DESC desc = d3d::GetBufferResourceDesc(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    _counter_buffer.Init(&desc);
}

void
StructuredBuffer::Free()
{
    // Garbage collect the resource
    CommandList *active_list = RendererGetActiveCommandList();
    active_list->DeleteResource(&_resource);
    ResourceStateTracker::RemoveGlobalResourceState(_resource._handle);
    
    //_resource.Free();
    _counter_buffer.Free();
}
