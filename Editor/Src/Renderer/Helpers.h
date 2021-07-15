#ifndef _HELPERS_H
#define _HELPERS_H

namespace d3d
{
    FORCE_INLINE D3D12_HEAP_PROPERTIES
        GetHeapProperties(D3D12_HEAP_TYPE type = D3D12_HEAP_TYPE_DEFAULT)
    {
        D3D12_HEAP_PROPERTIES result = {};
        result.Type                 = type;
        result.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        result.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        result.CreationNodeMask     = 1;
        result.VisibleNodeMask      = 1;
        return result;
    }
    
    FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE
        GetCpuDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, u32 inc_size, u32 offset = 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE result = {};
        result.ptr = handle.ptr + offset * inc_size;
        return result;
    }
    
    FORCE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE
        GetGpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle, u32 inc_size, u32 offset = 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE result = {};
        result.ptr = handle.ptr + offset * inc_size;
        return result;
    }
    
    FORCE_INLINE D3D12_RESOURCE_BARRIER 
        GetUAVBarrier(ID3D12Resource *resource)
    {
        D3D12_RESOURCE_BARRIER result = {};
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        result.UAV.pResource = resource;
        return result;
    }
    
    FORCE_INLINE D3D12_RESOURCE_BARRIER 
        GetAliasingBarrier(ID3D12Resource *before, ID3D12Resource *after)
    {
        D3D12_RESOURCE_BARRIER result = {};
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        result.Aliasing.pResourceBefore = before;
        result.Aliasing.pResourceAfter = after;
        return result;
    }
    
    FORCE_INLINE D3D12_RESOURCE_BARRIER 
        GetTransitionBarrier(ID3D12Resource* resource,
                             D3D12_RESOURCE_STATES before,
                             D3D12_RESOURCE_STATES after,
                             UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                             D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
    {
        D3D12_RESOURCE_BARRIER result = {};
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        result.Flags = flags;
        result.Transition.pResource = resource;
        result.Transition.StateBefore = before;
        result.Transition.StateAfter = after;
        result.Transition.Subresource = subresource;
        return result;
    }
    
    FORCE_INLINE D3D12_RESOURCE_DESC
        GetResourceDesc(D3D12_RESOURCE_DIMENSION dimension,
                        UINT64 alignment,
                        UINT64 width,
                        UINT height,
                        UINT16 depthOrArraySize,
                        UINT16 mipLevels,
                        DXGI_FORMAT format,
                        UINT sampleCount,
                        UINT sampleQuality,
                        D3D12_TEXTURE_LAYOUT layout,
                        D3D12_RESOURCE_FLAGS flags)
    {
        D3D12_RESOURCE_DESC result = {};
        result.Dimension = dimension;
        result.Alignment = alignment;
        result.Width = width;
        result.Height = height;
        result.DepthOrArraySize = depthOrArraySize;
        result.MipLevels = mipLevels;
        result.Format = format;
        result.SampleDesc.Count = sampleCount;
        result.SampleDesc.Quality = sampleQuality;
        result.Layout = layout;
        result.Flags = flags;
        return result;
    }
    
    FORCE_INLINE D3D12_RESOURCE_DESC
        GetBufferResourceDesc(D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
                              D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
    {
        return GetResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, resAllocInfo.Alignment, resAllocInfo.SizeInBytes,
                               1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
    }
    
    static FORCE_INLINE D3D12_RESOURCE_DESC GetTex2DDesc(DXGI_FORMAT format,
                                                         UINT64 width,
                                                         UINT height,
                                                         UINT16 arraySize = 1,
                                                         UINT16 mipLevels = 0,
                                                         UINT sampleCount = 1,
                                                         UINT sampleQuality = 0,
                                                         D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                                                         D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                         UINT64 alignment = 0 )
    {
        return GetResourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, alignment, width, height, arraySize, 
                               mipLevels, format, sampleCount, sampleQuality, layout, flags);
    }
    
    FORCE_INLINE D3D12_RESOURCE_DESC
        GetBufferResourceDesc(UINT64 width,
                              D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                              UINT64 alignment = 0)
    {
        return GetResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1,
                               DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
    }
    
    // SOURCE: d3dx12.h
    FORCE_INLINE void 
        MemcpySubresource(_In_ const D3D12_MEMCPY_DEST* pDest,
                          _In_ const D3D12_SUBRESOURCE_DATA* pSrc,
                          SIZE_T RowSizeInBytes,
                          UINT NumRows,
                          UINT NumSlices) noexcept
    {
        for (UINT z = 0; z < NumSlices; ++z)
        {
            auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
            auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
            for (UINT y = 0; y < NumRows; ++y)
            {
                memcpy(pDestSlice + pDest->RowPitch * y,
                       pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
                       RowSizeInBytes);
            }
        }
    }
    
    
    FORCE_INLINE void 
        MemcpySubresource(const D3D12_MEMCPY_DEST* pDest,
                          const void* pResourceData,
                          const D3D12_SUBRESOURCE_INFO* pSrc,
                          SIZE_T RowSizeInBytes,
                          UINT NumRows,
                          UINT NumSlices)
    {
        for (UINT z = 0; z < NumSlices; ++z)
        {
            auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
            auto pSrcSlice = (static_cast<const BYTE*>(pResourceData) + pSrc->Offset) + pSrc->DepthPitch * ULONG_PTR(z);
            for (UINT y = 0; y < NumRows; ++y)
            {
                memcpy(pDestSlice + pDest->RowPitch * y,
                       pSrcSlice + pSrc->RowPitch * ULONG_PTR(y),
                       RowSizeInBytes);
            }
        }
    }
    
    FORCE_INLINE D3D12_TEXTURE_COPY_LOCATION
        GetTextureCopyLoction(ID3D12Resource* pRes, UINT Sub)
    {
        D3D12_TEXTURE_COPY_LOCATION result = {};
        result.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        result.PlacedFootprint = {};
        result.SubresourceIndex = Sub;
        result.pResource = pRes;
        return result;
    }
    
    FORCE_INLINE D3D12_TEXTURE_COPY_LOCATION
        GetTextureCopyLoction(ID3D12Resource* pRes, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& Footprint)
    {
        D3D12_TEXTURE_COPY_LOCATION result = {};
        result.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        result.PlacedFootprint = Footprint;
        result.pResource = pRes;
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // All arrays must be populated (e.g. by calling GetCopyableFootprints)
    inline UINT64 UpdateSubresources(
                                     _In_ ID3D12GraphicsCommandList* pCmdList,
                                     _In_ ID3D12Resource* pDestinationResource,
                                     _In_ ID3D12Resource* pIntermediate,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
                                     UINT64 RequiredSize,
                                     _In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
                                     _In_reads_(NumSubresources) const UINT* pNumRows,
                                     _In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
                                     _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
    {
        // Minor validation
        auto IntermediateDesc = pIntermediate->GetDesc();
        auto DestinationDesc = pDestinationResource->GetDesc();
        if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
            RequiredSize > SIZE_T(-1) ||
            (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
             (FirstSubresource != 0 || NumSubresources != 1)))
        {
            return 0;
        }
        
        BYTE* pData;
        HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (FAILED(hr))
        {
            return 0;
        }
        
        for (UINT i = 0; i < NumSubresources; ++i)
        {
            if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
            D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
            MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
        }
        pIntermediate->Unmap(0, nullptr);
        
        if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            pCmdList->CopyBufferRegion(
                                       pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
        }
        else
        {
            for (UINT i = 0; i < NumSubresources; ++i)
            {
                
                D3D12_TEXTURE_COPY_LOCATION Dst = GetTextureCopyLoction(pDestinationResource, i + FirstSubresource);
                D3D12_TEXTURE_COPY_LOCATION Src = GetTextureCopyLoction(pIntermediate, pLayouts[i]);
                pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
            }
        }
        return RequiredSize;
    }
    
    //------------------------------------------------------------------------------------------------
    // All arrays must be populated (e.g. by calling GetCopyableFootprints)
    inline UINT64 UpdateSubresources(
                                     _In_ ID3D12GraphicsCommandList* pCmdList,
                                     _In_ ID3D12Resource* pDestinationResource,
                                     _In_ ID3D12Resource* pIntermediate,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
                                     UINT64 RequiredSize,
                                     _In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
                                     _In_reads_(NumSubresources) const UINT* pNumRows,
                                     _In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
                                     _In_ const void* pResourceData,
                                     _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_INFO* pSrcData) noexcept
    {
        // Minor validation
        auto IntermediateDesc = pIntermediate->GetDesc();
        auto DestinationDesc = pDestinationResource->GetDesc();
        if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
            RequiredSize > SIZE_T(-1) ||
            (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
             (FirstSubresource != 0 || NumSubresources != 1)))
        {
            return 0;
        }
        
        BYTE* pData;
        HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (FAILED(hr))
        {
            return 0;
        }
        
        for (UINT i = 0; i < NumSubresources; ++i)
        {
            if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
            D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
            MemcpySubresource(&DestData, pResourceData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
        }
        pIntermediate->Unmap(0, nullptr);
        
        if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            pCmdList->CopyBufferRegion(
                                       pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
        }
        else
        {
            for (UINT i = 0; i < NumSubresources; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION Dst = GetTextureCopyLoction(pDestinationResource, i + FirstSubresource);
                D3D12_TEXTURE_COPY_LOCATION Src = GetTextureCopyLoction(pIntermediate, pLayouts[i]);
                pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
            }
        }
        return RequiredSize;
    }
    
    //------------------------------------------------------------------------------------------------
    // Heap-allocating UpdateSubresources implementation
    inline UINT64 UpdateSubresources(
                                     _In_ ID3D12GraphicsCommandList* pCmdList,
                                     _In_ ID3D12Resource* pDestinationResource,
                                     _In_ ID3D12Resource* pIntermediate,
                                     UINT64 IntermediateOffset,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
                                     _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
    {
        UINT64 RequiredSize = 0;
        auto MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
        if (MemToAlloc > SIZE_MAX)
        {
            return 0;
        }
        void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
        if (pMem == nullptr)
        {
            return 0;
        }
        auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
        auto pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
        auto pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);
        
        auto Desc = pDestinationResource->GetDesc();
        ID3D12Device* pDevice = nullptr;
        pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
        pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
        pDevice->Release();
        
        UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
        HeapFree(GetProcessHeap(), 0, pMem);
        return Result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Heap-allocating UpdateSubresources implementation
    inline UINT64 UpdateSubresources(
                                     _In_ ID3D12GraphicsCommandList* pCmdList,
                                     _In_ ID3D12Resource* pDestinationResource,
                                     _In_ ID3D12Resource* pIntermediate,
                                     UINT64 IntermediateOffset,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
                                     _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
                                     _In_ const void* pResourceData,
                                     _In_reads_(NumSubresources) D3D12_SUBRESOURCE_INFO* pSrcData) noexcept
    {
        UINT64 RequiredSize = 0;
        auto MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
        if (MemToAlloc > SIZE_MAX)
        {
            return 0;
        }
        void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
        if (pMem == nullptr)
        {
            return 0;
        }
        auto pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
        auto pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
        auto pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);
        
        auto Desc = pDestinationResource->GetDesc();
        ID3D12Device* pDevice = nullptr;
        pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
        pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
        pDevice->Release();
        
        UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pResourceData, pSrcData);
        HeapFree(GetProcessHeap(), 0, pMem);
        return Result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Stack-allocating UpdateSubresources implementation
    template <UINT MaxSubresources>
        inline UINT64 UpdateSubresources(
                                         _In_ ID3D12GraphicsCommandList* pCmdList,
                                         _In_ ID3D12Resource* pDestinationResource,
                                         _In_ ID3D12Resource* pIntermediate,
                                         UINT64 IntermediateOffset,
                                         _In_range_(0,MaxSubresources) UINT FirstSubresource,
                                         _In_range_(1,MaxSubresources-FirstSubresource) UINT NumSubresources,
                                         _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
    {
        UINT64 RequiredSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
        UINT NumRows[MaxSubresources];
        UINT64 RowSizesInBytes[MaxSubresources];
        
        auto Desc = pDestinationResource->GetDesc();
        ID3D12Device* pDevice = nullptr;
        pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
        pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts, NumRows, RowSizesInBytes, &RequiredSize);
        pDevice->Release();
        
        return UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, Layouts, NumRows, RowSizesInBytes, pSrcData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Stack-allocating UpdateSubresources implementation
    template <UINT MaxSubresources>
        inline UINT64 UpdateSubresources(
                                         _In_ ID3D12GraphicsCommandList* pCmdList,
                                         _In_ ID3D12Resource* pDestinationResource,
                                         _In_ ID3D12Resource* pIntermediate,
                                         UINT64 IntermediateOffset,
                                         _In_range_(0,MaxSubresources) UINT FirstSubresource,
                                         _In_range_(1,MaxSubresources-FirstSubresource) UINT NumSubresources,
                                         _In_ const void* pResourceData,
                                         _In_reads_(NumSubresources) D3D12_SUBRESOURCE_INFO* pSrcData) noexcept
    {
        UINT64 RequiredSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
        UINT NumRows[MaxSubresources];
        UINT64 RowSizesInBytes[MaxSubresources];
        
        auto Desc = pDestinationResource->GetDesc();
        ID3D12Device* pDevice = nullptr;
        pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
        pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts, NumRows, RowSizesInBytes, &RequiredSize);
        pDevice->Release();
        
        return UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, Layouts, NumRows, RowSizesInBytes, pResourceData, pSrcData);
    }
    
    //------------------------------------------------------------------------------------------------
    // Returns required size of a buffer to be used for data upload
    FORCE_INLINE UINT64 
        GetRequiredIntermediateSize(_In_ ID3D12Resource* pDestinationResource,
                                    _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
                                    _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources)
    {
        auto Desc = pDestinationResource->GetDesc();
        UINT64 RequiredSize = 0;
        
        ID3D12Device* pDevice = nullptr;
        pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
        pDevice->Release();
        
        return RequiredSize;
    }
    
    //------------------------------------------------------------------------------------------------
    // Descriptor Ranges
    
    FORCE_INLINE D3D12_DESCRIPTOR_RANGE1 
        GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
                            UINT numDescriptors,
                            UINT baseShaderRegister,
                            UINT registerSpace = 0,
                            D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
                            UINT offsetInDescriptorsFromTableStart =
                            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
    {
        D3D12_DESCRIPTOR_RANGE1 result = {};
        result.RangeType = rangeType;
        result.NumDescriptors = numDescriptors;
        result.BaseShaderRegister = baseShaderRegister;
        result.RegisterSpace = registerSpace;
        result.Flags = flags;
        result.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    // Root Parameters
    
    namespace root_param1
    {
        FORCE_INLINE D3D12_ROOT_PARAMETER1
            InitAsConstant(UINT num32BitValues,
                           UINT shaderRegister,
                           UINT registerSpace = 0,
                           D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        {
            D3D12_ROOT_PARAMETER1 result = {};
            result.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            result.ShaderVisibility = visibility;
            result.Constants.Num32BitValues = num32BitValues;
            result.Constants.ShaderRegister = shaderRegister;
            result.Constants.RegisterSpace = registerSpace;
            return result;
        }
        
        FORCE_INLINE D3D12_ROOT_PARAMETER1
            InitAsDescriptorTable(UINT numDescriptorRanges,
                                  D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges,
                                  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        {
            D3D12_ROOT_PARAMETER1 result = {};
            result.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            result.ShaderVisibility = visibility;
            result.DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
            result.DescriptorTable.pDescriptorRanges = pDescriptorRanges;
            return result;
        }
        
    } // root_param1
    
    
    //------------------------------------------------------------------------------------------------
    // Static sampler
    
    FORCE_INLINE D3D12_STATIC_SAMPLER_DESC
        GetStaticSamplerDesc(UINT shaderRegister,
                             D3D12_FILTER filter = D3D12_FILTER_ANISOTROPIC,
                             D3D12_TEXTURE_ADDRESS_MODE addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                             D3D12_TEXTURE_ADDRESS_MODE addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                             D3D12_TEXTURE_ADDRESS_MODE addressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                             FLOAT mipLODBias = 0,
                             UINT maxAnisotropy = 16,
                             D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
                             D3D12_STATIC_BORDER_COLOR borderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                             FLOAT minLOD = 0.f,
                             FLOAT maxLOD = D3D12_FLOAT32_MAX,
                             D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL, 
                             UINT registerSpace = 0)
    {
        D3D12_STATIC_SAMPLER_DESC result = {};
        result.ShaderRegister = shaderRegister;
        result.Filter = filter;
        result.AddressU = addressU;
        result.AddressV = addressV;
        result.AddressW = addressW;
        result.MipLODBias = mipLODBias;
        result.MaxAnisotropy = maxAnisotropy;
        result.ComparisonFunc = comparisonFunc;
        result.BorderColor = borderColor;
        result.MinLOD = minLOD;
        result.MaxLOD = maxLOD;
        result.ShaderVisibility = shaderVisibility;
        result.RegisterSpace = registerSpace;
        return result;
    }
    
} // d3d

#endif //_HELPERS_H
