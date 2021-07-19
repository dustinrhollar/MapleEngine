
void 
Resource::Init(D3D12_RESOURCE_DESC *resourceDesc, D3D12_CLEAR_VALUE* clearValue)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    if (clearValue)
    {
        memcpy(&_clear_value, clearValue, sizeof(D3D12_CLEAR_VALUE));
        _has_clear_val = true;
    }
    
    D3D12_HEAP_PROPERTIES prop = d3d::GetHeapProperties();
    
    D3D12_CLEAR_VALUE *val = (clearValue) ? &_clear_value : 0;
    
    AssertHr(d3d_device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, resourceDesc,
                                                 D3D12_RESOURCE_STATE_COMMON, val, IIDE(&_handle)));
    ResourceStateTracker::AddGlobalResourceState(_handle, D3D12_RESOURCE_STATE_COMMON);
    CheckFeatureSupport();
}

void 
Resource::Init(ID3D12Resource *resource, D3D12_CLEAR_VALUE* clearValue)
{
    _handle = resource;
    if (clearValue)
    {
        memcpy(&_clear_value, clearValue, sizeof(D3D12_CLEAR_VALUE));
    }
    CheckFeatureSupport();
}

void 
Resource::Free()
{
    if (_handle) D3D_RELEASE(_handle);
    _format_support = {};
    _has_clear_val = false;
    _clear_value = {};
}

// Check if the resource format supports a specific feature.
bool 
Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
    return (_format_support.Support1 & formatSupport) != 0;
}

bool 
Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport)
{
    return (_format_support.Support2 & formatSupport) != 0;
}

// Check the format support and populate the _format_support structure.
void 
Resource::CheckFeatureSupport()
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    D3D12_RESOURCE_DESC  desc = _handle->GetDesc();
    _format_support.Format = desc.Format;
    AssertHr(d3d_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &_format_support,
                                             sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
}

void 
Resource::Reset()
{
    D3D_RELEASE(_handle);
}
