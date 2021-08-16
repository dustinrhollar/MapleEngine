#ifndef _ASSET_MANAGER_H
#define _ASSET_MANAGER_H

struct AssetMetadata
{
    FILE_ID     file;
    Str         virtual_name; // unique name for asset. used only by the editor
    MAPLE_GUID  guid;         // unique 128bit identifier
    MAPLE_GUID  icon;         // @optional
    MAPLE_GUID *dependencies; // list of dependencies by GUID
    // TODO(Dustin): Versioning
};

enum AssetType
{
    Asset_Texture,
    
    Asset_Count,
};

//
// Internal repesentation of an asset. Used to unify storage
// for assets. Maintains a ref count for the number of held
// references to that asset at runtime. Requesting an ASSET_ID
// does incremenets the ref-count, so that user does not have to
// hold a pointer to the actual asset. Please see GetAsset and 
// ReleaseAsset in the AssetManager for more details.
//
struct Asset
{
    AssetType      type;
    // NOTE(Dustin): I almost want to store this u32 in its own separate storage...
    union
    {
        TEXTURE_ID tex;          // Texture asset
    };
};

union AssetRefCount
{
    struct 
    {
        u32 ref_count:30; // Number of references held for this asset
        u32 meta_dirty:1; // does the metadata need to be re-serialized?
        u32 dirty:1;      // does the asset need to be re-serialized?
    };
    u32 mask;
};

union ASSET_ID
{
    struct { u32 gen:8; u32 idx:24; };
    u32 mask;
};

struct VirtNameKeyValue
{
    char    *key;
    ASSET_ID value;
};

struct GuidKeyValue
{
    MAPLE_GUID key;
    ASSET_ID   value;
};

struct AssetManager
{
    AssetRefCount    *asset_refcounts;
    AssetMetadata    *asset_meta;
    Asset            *asset_storage; // backing memory for all assets
    u8               *gens;          // generational index
    u32              *free_indices;  // free slots in the asset storage
    
    VirtNameKeyValue *virtual_name_table; // map virtual name -> ASSET_ID
    GuidKeyValue     *guid_table;         // map GUID -> ASSET_ID
    
    void Init();
    void Shutdown();
    
    
};

static void SerializeMetafile(AssetMetadata metadata);
static void DeserializeMetafile(AssetMetadata *metadata, FILE_ID fid);

#endif //_ASSET_MANAGER_H
