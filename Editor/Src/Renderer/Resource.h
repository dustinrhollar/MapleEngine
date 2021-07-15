#ifndef _RESOURCE_H
#define _RESOURCE_H

struct Resource
{
    void Init(D3D12_RESOURCE_DESC* resourceDesc,
              D3D12_CLEAR_VALUE* clearValue = 0);
    void Init(ID3D12Resource *resource, 
              D3D12_CLEAR_VALUE* clearValue = 0);
    void Free();
    
    // Check if the resource format supports a specific feature.
    bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport);
    bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport);
    // Check the format support and populate the _format_support structure.
    void CheckFeatureSupport();
    
    void Reset();
    
    bool IsValid() { return (_handle) ? true : false; }
    void GetClearValue(D3D12_CLEAR_VALUE **clear_val)
    {
        *clear_val = (_has_clear_val) ? &_clear_value : 0;
    }
    
    D3D12_RESOURCE_DESC GetResourceDesc()
    {
        D3D12_RESOURCE_DESC rsrc_desc = {};
        if (_handle)
        {
            rsrc_desc = _handle->GetDesc();
        }
        return rsrc_desc;
    }
    
    // @INTERNAL
    
    ID3D12Resource                   *_handle = 0;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT _format_support;
    
    b8                                _has_clear_val = false;
    D3D12_CLEAR_VALUE                 _clear_value;
};

struct ShaderResourceView
{
    void Init(Resource *resource,
              D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr)
    {
        ID3D12Device *d3d_device = device::GetDevice();
        ID3D12Resource *d3d_rsrc = (resource) ? resource->_handle : 0;
        
        _resource = resource;
        _descriptor = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        d3d_device->CreateShaderResourceView(d3d_rsrc, srv, _descriptor.GetDescriptorHandle());
    }
    
    void Free()
    {
        _resource = 0;
        _descriptor.Free();
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return _descriptor.GetDescriptorHandle();
    }
    
    Resource            *_resource;
    DescriptorAllocation _descriptor;
};

struct UnorderedAccessView
{
    void Init(Resource                         *resource,
              Resource                         *counter_resource = 0,
              D3D12_UNORDERED_ACCESS_VIEW_DESC *uav              = 0)
    {
        ID3D12Device *d3d_device = device::GetDevice();
        ID3D12Resource *d3d_rsrc = (resource) ? resource->_handle : 0;
        ID3D12Resource *d3d_counter_rsrc = (counter_resource) ? counter_resource->_handle : 0;
        
        _resource = resource;
        _counter_resource = counter_resource;
        
        if (_resource)
        {
            D3D12_RESOURCE_DESC desc = _resource->GetResourceDesc();
            assert((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
        }
        
        _descriptor = device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        d3d_device->CreateUnorderedAccessView(d3d_rsrc, d3d_counter_rsrc, uav, _descriptor.GetDescriptorHandle());
    }
    
    void Free()
    {
        _resource = 0;
        _counter_resource = 0;
        _descriptor.Free();
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return _descriptor.GetDescriptorHandle();
    }
    
    Resource            *_resource;
    Resource            *_counter_resource;
    DescriptorAllocation _descriptor;
    
};

struct ConstantBuffer
{
    void Init(ID3D12Resource *resource)
    {
        _resource.Init(resource);
        _size = _resource.GetResourceDesc().Width;
    }
    
    void Free()
    {
        _resource.Free();
        _size = 0;
    }
    
    Resource _resource;
    u64      _size;
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
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return _descriptor.GetDescriptorHandle();
    }
    
    ConstantBuffer      *_cb;
    DescriptorAllocation _descriptor;
};

#endif //_RESOURCE_H
