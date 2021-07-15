#ifndef _RESOURCE_STATE_TRACKER_H
#define _RESOURCE_STATE_TRACKER_H

//
// In order to synchronize the state resources are in across multiple threads,
// we have to be careful about transitioning resources. To do this, the following
// data structures are need:
//
// - Pending ResourceTransition Barriers Array: a resource is bound to a command list
//   for the first time, where its previous state is unknown and the desired state is
//   known. These are NOT split barriers.
//
// - Final Resource State Map: a resource that has been used in the command list at
//   once, its final known state is added to the Final Resource State Map.
//
// - Resource Transition Barriers Array: if the current state is known (exists within
//   the Final Resource State Map), then any transition is added to this array. The array
//   is then committed to a command list before a Draw/Dispatch or any command that
//   required resource barriers to be flushed.
//
// - Global Resource State Map: A command may contain any number of state transitions
//   for a resource. When a command list is exeecuted on a command queue, the last known
//   state of a resource is saved in the global resource state map. It is this map that
//   is used to determine if a Pending Resource Transition Barrier is added to an
//   intermediate command list.
//

//
// The Resource State Tracker tracks the state of a resource within a single command
// list. Since it is not exepected that a command list is shared across threads during
// comamand list recording, it is assumed the same will be true for the Resource
// State Tracker.
//

struct ResourceStateTracker
{
    void Init();
    void Free();
    
    // Push a resource barrier
    void ResourceBarrier(D3D12_RESOURCE_BARRIER *barrier);
    
    // Push a transition resource barier
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    
    // Push a UAV Barrier for a given resource
    // @param resource: if null, then any UAV access could require the barrier
    void UAVBarrier(ID3D12Resource* resource = 0);
    
    // Push an aliasing barrier for a given resource.
    // @param: both can be null, which indicates that any placed/reserved resource could cause aliasing
    void AliasBarrier(ID3D12Resource* resource_before = 0, ID3D12Resource* resource_after = 0);
    
    // Flush any pending resource barriers to the command list
    u32 FlushPendingResourceBarriers(struct CommandList *cmd_list);
    
    // Flush any (non-pending) resource barriers that have been pushed to the resource state tracker.
    void FlushResourceBarriers(struct CommandList *cmd_list);
    
    // Commit final resource states to the global resource state map. This must be called when
    // the command list is closed.
    void CommitFinalResourceStates();
    
    // Reset resource state tracking. Done when command list is reset.
    void Reset();
    
    // Global state must be locked before flushing pending resource barriers and committing 
    // the final resource state to the global resource state. This ensures consistency of 
    // the global resource state between command list exectuions.
    static void Lock();
    // Unlocks the global resource state after barriers have been committed.
    static void Unlock();
    
    // Add a resource to the global state. Done when resource is first created.
    static void AddGlobalResourceState(ID3D12Resource *resource, D3D12_RESOURCE_STATES state);
    // Removes a resource from the global state map. Done when a resource is destroyed.
    static void RemoveGlobalResourceState(ID3D12Resource *resource);
    
    // @INTERNAL
    
    using ResourceBarriers = D3D12_RESOURCE_BARRIER*; // stb array
    
    struct ResourceState 
    {
        void Init(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
        {
            _state = state;
            _subresource_state = 0;
        }
        
        void Free()
        {
            arrfree(_subresource_state);
        }
        
        void SetSubresourceState(UINT subresource, D3D12_RESOURCE_STATES state)
        {
            if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                _state = state;
                arrsetlen(_subresource_state, 0);
            }
            else
            {
                // Try to find the subresource, if not found add to list
                bool found = false;
                for (u32 i = 0; i < (u32)arrlen(_subresource_state); ++i)
                {
                    if (_subresource_state[i].id == subresource)
                    {
                        _subresource_state[i].state = state;
                        found = true;
                        break;
                    }
                }
                
                if (!found)
                {
                    Subresource new_rsrc = {};
                    new_rsrc.id = subresource;
                    new_rsrc.state = state;
                    arrput(_subresource_state, new_rsrc);
                }
            }
        }
        
        D3D12_RESOURCE_STATES GetSubresourceState(UINT subresource)
        {
            D3D12_RESOURCE_STATES result = _state;
            for (u32 i = 0; i < (u32)arrlen(_subresource_state); ++i)
            {
                if (_subresource_state[i].id == subresource)
                {
                    result = _subresource_state[i].state;
                    break;
                }
            }
            return result;
        }
        
        D3D12_RESOURCE_STATES _state;
        // Could use a map, but I don't think there will ever be enough
        // subresources to justify the extra overhead. A quick linear scan
        // *should* be fast enough to find a subresource.
        struct Subresource
        {
            UINT                  id;
            D3D12_RESOURCE_STATES state;
        } *_subresource_state = 0;
    };
    
    struct ResourceStateMap
    {
        ID3D12Resource* key;  
        ResourceState   value;
    };
    
    // Pending resource transition are committed before a command list is executed
    // on the command queue. This guarentees that resource will be in the expected
    // state at the beginning of the command list.
    ResourceBarriers         _pending_resource_barriers = 0;
    // Resource barriers that need to be committed to the command list.
    ResourceBarriers         _resource_barriers = 0;
    // The last known state of the resource within the command list. The final
    // resource state is committed to the global resource state when the command
    // list is closed, but before it is exevuted on the command queue.
    ResourceStateMap        *_final_resource_state = 0;
    // Global resource state
    // The global resource state map stores the state of a resource between
    // command list execution
    static ResourceStateMap *_s_global_resource_state;
    static CRITICAL_SECTION  _s_cs_global_lock;
    static bool              _s_is_locked;
};

#endif //_RESOURCE_STATE_TRACKER_H
