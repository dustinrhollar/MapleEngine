
ResourceStateTracker::ResourceStateMap *ResourceStateTracker::_s_global_resource_state = 0;
CRITICAL_SECTION  ResourceStateTracker::_s_cs_global_lock; // Need to initialize?
bool              ResourceStateTracker::_s_is_locked = false;

static void 
InitalizeGlobalResourceState()
{
    InitializeCriticalSectionAndSpinCount(&ResourceStateTracker::_s_cs_global_lock, 1024);
}

static void 
FreeGlobalResourceState()
{
    hmfree(ResourceStateTracker::_s_global_resource_state);
    DeleteCriticalSection(&ResourceStateTracker::_s_cs_global_lock);
}

void 
ResourceStateTracker::Init()
{
    _pending_resource_barriers = 0;
    _resource_barriers = 0;
    _final_resource_state = 0;
}

void 
ResourceStateTracker::Free()
{
    // TODO(Dustin):
}

// Push a resource barrier
void 
ResourceStateTracker::ResourceBarrier(D3D12_RESOURCE_BARRIER *barrier)
{
    if (barrier->Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
    {
        D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier = barrier->Transition;
        
        // First check if there is already a known "final" state for the given resource.
        // If there is, the resource has been used on the command list before and
        // already has a known state within the command list execution.
        i64 rsrc_idx = -1;
        if ((rsrc_idx = hmgeti(_final_resource_state, transitionBarrier.pResource)) != -1)
        {
            ResourceState *resource_state = &_final_resource_state[rsrc_idx].value;
            
            // If the known final state of the resource is different...
            if (transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                (arrlen(resource_state->_subresource_state) > 0))
            {
                // First transition all of the subresources if they are different than the StateAfter.
                for (u32 i = 0; i < (u32)arrlen(resource_state->_subresource_state); ++i)
                {
                    ResourceState::Subresource subresource_state = resource_state->_subresource_state[i];
                    
                    if (transitionBarrier.StateAfter != subresource_state.state)
                    {
                        D3D12_RESOURCE_BARRIER new_barrier = *barrier;
                        new_barrier.Transition.Subresource = subresource_state.id;
                        new_barrier.Transition.StateBefore = subresource_state.state;
                        arrput(_resource_barriers, new_barrier);
                    }
                }
            }
            else
            {
                D3D12_RESOURCE_STATES final_state = resource_state->GetSubresourceState(transitionBarrier.Subresource);
                if (transitionBarrier.StateAfter != final_state)
                {
                    // Push a new transition barrier with the correct before state.
                    D3D12_RESOURCE_BARRIER new_barrier = *barrier;
                    new_barrier.Transition.StateBefore = final_state;
                    arrput(_resource_barriers, new_barrier);
                }
            }
            
            resource_state->SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
        }
        else
        { // In this case, the resource is being used on the command list for the first time. 
            // Add a pending barrier. The pending barriers will be resolved
            // before the command list is executed on the command queue.
            arrput(_pending_resource_barriers, *barrier);
            
            ResourceState new_state = {};
            new_state.Init();
            new_state.SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
            
            ResourceStateMap entry = {};
            entry.key   = transitionBarrier.pResource;  
            entry.value = new_state;
            hmputs(_final_resource_state, entry);
        }
    }
    else
    {
        arrput(_resource_barriers, *barrier);
    }
}

// Push a transition resource barier
void 
ResourceStateTracker::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_after, UINT subResource)
{
    if (resource)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = state_after;
        barrier.Transition.Subresource = subResource;
        ResourceBarrier(&barrier);
    }
}

// Push a UAV Barrier for a given resource
// @param resource: if null, then any UAV access could require the barrier
void 
ResourceStateTracker::UAVBarrier(ID3D12Resource* resource)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;
    ResourceBarrier(&barrier);
}

// Push an aliasing barrier for a given resource.
// @param: both can be null, which indicates that any placed/reserved resource could cause aliasing
void 
ResourceStateTracker::AliasBarrier(ID3D12Resource* resource_before, ID3D12Resource* resource_after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Aliasing.pResourceBefore = resource_before;
    barrier.Aliasing.pResourceAfter = resource_after;
    ResourceBarrier(&barrier);
}

// Flush any pending resource barriers to the command list
u32 
ResourceStateTracker::FlushPendingResourceBarriers(CommandList *cmd_list)
{
    assert(_s_is_locked && "ResourceStateTracker::FlushPendingResourceBarriers: Global should be locked before calling.");
    
    // Resolve the pending resource barriers by checking the global state of the 
    // (sub)resources. Add barriers if the pending state and the global state do
    //  not match.
    ResourceBarriers resource_barriers = 0;
    arrsetcap(resource_barriers, arrlen(_pending_resource_barriers));
    
    for (u32 i = 0; i < (u32)arrlen(_pending_resource_barriers); ++i)
    {
        D3D12_RESOURCE_BARRIER pending_barrier = _pending_resource_barriers[i];
        
        if (pending_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)  // Only transition barriers should be pending...
        {
            D3D12_RESOURCE_TRANSITION_BARRIER pending_transition = pending_barrier.Transition;
            
            //const auto& iter = ms_GlobalResourceState.find(pending_transition.pResource);
            i64 global_rsrc_idx = -1;
            if ((global_rsrc_idx = hmgeti(_s_global_resource_state, pending_transition.pResource)) != -1)
            {
                
                // If all subresources are being transitioned, and there are multiple
                // subresources of the resource that are in a different state...
                ResourceState resource_state = _s_global_resource_state[global_rsrc_idx].value;
                if ( pending_transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                    (arrlen(resource_state._subresource_state) > 0))
                {
                    // Transition all subresources
                    for (u32 i = 0; i < (u32)arrlen(resource_state._subresource_state); ++i)
                    {
                        ResourceState::Subresource subresource_state = resource_state._subresource_state[i];
                        if ( pending_transition.StateAfter != subresource_state.state)
                        {
                            D3D12_RESOURCE_BARRIER new_barrier = pending_barrier;
                            new_barrier.Transition.Subresource = subresource_state.id;
                            new_barrier.Transition.StateBefore = subresource_state.state;
                            arrput(resource_barriers, new_barrier);
                        }
                    }
                }
                else
                {
                    // No (sub)resources need to be transitioned. Just add a single transition barrier (if needed).
                    D3D12_RESOURCE_STATES global_state = resource_state.GetSubresourceState(pending_transition.Subresource);
                    if (pending_transition.StateAfter != global_state )
                    {
                        // Fix-up the before state based on current global state of the resource.
                        pending_barrier.Transition.StateBefore = global_state;
                        arrput(resource_barriers, pending_barrier);
                    }
                }
            }
        }
    }
    
    UINT num_barriers = (UINT)arrlen(resource_barriers);
    if (num_barriers > 0 )
    {
        cmd_list->_handle->ResourceBarrier(num_barriers, resource_barriers);
    }
    
    arrsetlen(_pending_resource_barriers, 0);
    arrfree(resource_barriers);
    
    return num_barriers;
}

// Flush any (non-pending) resource barriers that have been pushed to the resource state tracker.
void 
ResourceStateTracker::FlushResourceBarriers(CommandList *cmd_list)
{
    UINT num_barriers = (UINT)arrlen(_resource_barriers);
    if (num_barriers > 0 )
    {
        cmd_list->_handle->ResourceBarrier(num_barriers, _resource_barriers);
        arrsetlen(_resource_barriers, 0);
    }
}

// Commit final resource states to the global resource state map. This must be called when
// the command list is closed.
void 
ResourceStateTracker::CommitFinalResourceStates()
{
    assert(_s_is_locked && "ResourceStateTracker::CommitFinalResourceStates: Global should be locked before calling.");
    
    // Commit final resource states to the global resource state array (map).
    for (u32 i = 0; i < (u32)hmlenu(_final_resource_state); ++i)
    {
        ResourceStateMap entry = _final_resource_state[i];
        hmputs(_s_global_resource_state, entry);
    }
    
    // NOTE(Dustin): This is rather inefficeint....
    // Find a better way to clear the hashmap
    hmfree(_final_resource_state);
}

// Reset resource state tracking. Done when command list is reset.
void 
ResourceStateTracker::Reset()
{
    arrsetlen(_pending_resource_barriers, 0);
    arrsetlen(_resource_barriers, 0);
    
    // NOTE(Dustin): This is rather inefficeint....
    // Find a better way to clear the hashmap
    for (u32 i = 0; i < (u32)hmlen(_final_resource_state); ++i)
    {
        D3D_RELEASE(_final_resource_state[i].key);
        arrfree(_final_resource_state[i].value._subresource_state);
    }
    hmfree(_final_resource_state);
}

// Global state must be locked before flushing pending resource barriers and committing 
// the final resource state to the global resource state. This ensures consistency of 
// the global resource state between command list exectuions.
void 
ResourceStateTracker::Lock()
{
    EnterCriticalSection(&_s_cs_global_lock);
    _s_is_locked = true;
}

// Unlocks the global resource state after barriers have been committed.
void 
ResourceStateTracker::Unlock()
{
    LeaveCriticalSection(&_s_cs_global_lock);
    _s_is_locked = false;
}

// Add a resource to the global state. Done when resource is first created.
void 
ResourceStateTracker::AddGlobalResourceState(ID3D12Resource *resource, D3D12_RESOURCE_STATES state)
{
    if (resource != NULL)
    {
        ResourceStateMap entry = {};
        entry.key = resource;  
        entry.value.Init();
        entry.value.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
        
        Lock();
        hmputs(_s_global_resource_state, entry);
        Unlock();
    }
}

// Removes a resource from the global state map. Done when a resource is destroyed.
void 
ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource *resource)
{
    if ( resource != nullptr )
    {
        Lock();
        hmdel(_s_global_resource_state, resource);
        Unlock();
    }
}
