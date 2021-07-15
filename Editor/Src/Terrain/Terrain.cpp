
// TODO(Dustin): 
// - Set model matrix
// - Render routine
// - Terrain settings panel
// - Draw terrain into main viewport
// - Viewport camera controls

#include "TerrainFunctions.cpp"
#include "HeightmapDownSampler.cpp"

enum class TerrainMeshType : u8
{
    Standard,      // simple mesh generation
    TriangleStrip, // optimized for low poly count
    Pizza,         // optmized for low poly count for DLOD
    LowPoly,       // optimized for low poly normals with low memory overhead
    TIN,           // optimizaed for mesh detail
};

struct TerrainTileInfo
{
    TerrainMeshType meshing_strategy;
    u32 tile_x = 0;            // # of tiles in the x direction
    u32 tile_y = 0;            // # of tiles in the y direction
    u32 vertex_x = 0;          // # of vertices in the x direction
    u32 vertex_y = 0;          // # of vertices in the y direction
    r32 scale = 0.0f;          // Scale of a tile
    r32 texture_tiling = 0.0f; // Heightmap tiling on a tile
};

struct TerrainTile
{
    // NOTE(Dustin): With the exception of the TIN mesh
    // tiles can be drawn in instances, so they don't need
    // to store unique vertex/index buffers
    
    VertexBuffer _vbuffer;
    IndexBuffer  _ibuffer;
    TEXTURE_ID   _heightmap_texture;
    m4           _model;
    
    void Free();
    
    // @param pos:   x,z position on the terrain grid
    // @param scale: x,z scale for the tile
    void SetModelMatrix(v2 pos, r32 scale);
    
    // @param use_as_texture: a flag to determine if the hightmap is sampled in the shader
    //  or in is the height embedded into the vertex.
    void AttachTexture(TEXTURE_ID heightmap_texture, bool use_as_texture = true);
    void GenMesh(CommandList *command_list, TerrainMeshType meshing_strategy, 
                 u32 width, u32 height, r32 tiling, r32 scale);
    
    void Render(CommandList *command_list);
};

struct Terrain
{
    void Init();
    void Free();
    
    // @param meshing_strategy: type of meshing that will be used to generate each tile mesh
    // @param heightmap_list:   list of heightmaps (_tile_x_count * _tile_y_count) in size
    //                          it is expected that the heightmaps are organized in row-major order
    void Generate(CommandList *command_list, TerrainTileInfo *tile_info, TEXTURE_ID *heightmap_list);
    
    // @param command_list: command list to record commands into
    // @param proj_view:    Porjection - View Matrix
    void Render(CommandList *command_list, m4 proj_view, TEXTURE_ID heightmap);
    
    RootSignature       _root_signature;
    PipelineStateObject _pso_solid;
    PipelineStateObject _pso_wireframe;
    // Render with wireframe
    bool                _wireframe_mode;
    /* Maximum amount of tiles in x & y direction */
    //u32                 _tile_x_count;
    //u32                 _tile_y_count;
    /* Vertex width/height of an individual tile */
    //u32                 _tile_width;
    //u32                 _tile_height;
    /* List of tiles of (_tile_x_count * _tile_y_count) size */
    TerrainTileInfo     _tile_info;
    TerrainTile        *_tiles = 0;
    
    // Holding onto vertex/index data for now.
    //void *_vertex_data;
    //void *_index_data;
    
    void SetWireframe(bool set) { _wireframe_mode = set; }
};

namespace terrain
{
    // Terrain vertex that does not include the height componenet
    struct TerrainVertex
    {
        v2 pos;   // position (x,z)
        v3 norms; // normals
        v2 uvs;   // tex coords
    };
    
    // Terrain vertex that does include the height componenet
    struct TerrainVertexWithHeight
    {
        v3 pos;   // position (x,y,z)
        v3 norms; // normals
        v2 uvs;   // tex coords
    };
    
    struct TerrainGenInfo
    {
        u32 width;
        u32 height;
        // texture tiling on a mesh tile
        r32 tiling;
    };
    
    enum RootParameters
    {
        MatrixCB,
        HeightmapTexture,
        NumParams,
    };
    
    static wchar_t *g_vertex_shader = L"shaders/TerrainVertex.cso";
    static wchar_t *g_pixel_shader  = L"shaders/TerrainPixel.cso";
    
    static GfxInputElementDesc g_vertex_no_height_input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    static GfxInputElementDesc g_vertex_height_input_desc[] = {
        { "POSITION", 0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "NORMAL",   0, GfxFormat::R32G32B32_Float, 0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 },
        { "TEXCOORD", 0, GfxFormat::R32G32_Float,    0, D3D12_APPEND_ALIGNED_ELEMENT, GfxInputClass::PerVertex, 0 }
    };
    
    // @param vtx_buffer: (output) final vertex buffer
    // @param idx_buffer: (output) final index buffer
    static void GenerateSimpleGrid(VertexBuffer *vtx_buffer, IndexBuffer *idx_buffer,
                                   CommandList *command_list, TerrainGenInfo *gen_info);
    
    static void GenerateTriangleStripGrid(VertexBuffer *vtx_buffer, IndexBuffer *idx_buffer,
                                          CommandList *command_list, TerrainGenInfo *gen_info);
    
    static void GenerateLowPolyGrid();
    static void GeneratePizzaGrid();
    
}

void 
Terrain::Init()
{
    //---------------------------------------------------------------------------------------------
    // Create Root Signature
    
    D3D12_ROOT_PARAMETER1 matrix_param = d3d::root_param1::InitAsConstant(sizeof(m4) / 4, 0, 0,
                                                                          D3D12_SHADER_VISIBILITY_VERTEX);
    
    D3D12_DESCRIPTOR_RANGE1 texture_range = d3d::GetDescriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    D3D12_ROOT_PARAMETER1 height_param = d3d::root_param1::InitAsDescriptorTable(1, &texture_range, 
                                                                                 D3D12_SHADER_VISIBILITY_VERTEX);
    
    D3D12_ROOT_PARAMETER1 root_params[terrain::RootParameters::NumParams];
    root_params[terrain::RootParameters::MatrixCB]         = matrix_param;
    root_params[terrain::RootParameters::HeightmapTexture] = height_param;
    
    // Diffuse texture sampler
    D3D12_STATIC_SAMPLER_DESC linear_sampler = d3d::GetStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    
    _root_signature.Init(_countof(root_params), root_params, 1, &linear_sampler, root_sig_flags);
    
    //---------------------------------------------------------------------------------------------
    // Create Pipeline State object
    
    GfxShaderModules shader_modules;
    shader_modules.vertex = LoadShaderModule(terrain::g_vertex_shader);
    shader_modules.pixel  = LoadShaderModule(terrain::g_pixel_shader);
    
    GfxPipelineStateDesc pso_desc{};
    pso_desc.root_signature      = &_root_signature;
    pso_desc.shader_modules      = shader_modules;
    pso_desc.blend               = GetBlendState(BlendState::Disabled);
    pso_desc.depth               = GetDepthStencilState(DepthStencilState::ReadWrite);
    pso_desc.rasterizer          = GetRasterizerState(RasterState::Default);
    pso_desc.input_layouts       = &terrain::g_vertex_no_height_input_desc[0];
    pso_desc.input_layouts_count = _countof(terrain::g_vertex_no_height_input_desc);
    pso_desc.topology            = GfxTopology::Triangle;
    pso_desc.sample_desc.count   = 1;
    // @FIXME probably should grab the swapchain manually like this...
    pso_desc.rtv_formats         = g_swapchain.GetRenderTarget()->GetRenderTargetFormats();
    pso_desc.dsv_format          = GfxFormat::D32_Float;
    
    _pso_solid.Init(&pso_desc);
    
    pso_desc.rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    _pso_wireframe.Init(&pso_desc);
    
    //---------------------------------------------------------------------------------------------
    // Finish up...
    
    _tile_info = {};
    // Don't initialize the tile list. This will let us know if there
    // has been a set of tiles allocated yet
    _tiles = 0;
    _wireframe_mode = true;
}

void 
Terrain::Free()
{
    _pso_solid.Free();
    _pso_wireframe.Free();
    _root_signature.Free();
    
    u32 tile_count = _tile_info.tile_x * _tile_info.tile_y;
    if (_tiles)
    {
        for (u32 i = 0; i < tile_count; ++i)
            _tiles[i].Free();
    }
}

// @param meshing_strategy: type of meshing that will be used to generate each tile mesh
// @param heightmap_list:   list of heightmaps (_tile_x_count * _tile_y_count) in size
//                          it is expected that the heightmaps are organized in row-major order
void
Terrain::Generate(CommandList *command_list, TerrainTileInfo *tile_info, TEXTURE_ID *heightmap_list)
{
    if (_tiles)
    {
        for (u32 j = 0; j < _tile_info.tile_y; ++j)
        {
            TerrainTile *row = _tiles + (j * _tile_info.tile_x);
            for (u32 i = 0; i < _tile_info.tile_x; ++i)
            {
                row[i].Free();
            }
        }
    }
    
    tile_info->tile_x = fast_max(tile_info->tile_x, 1);
    tile_info->tile_y = fast_max(tile_info->tile_y, 1);
    u32 tile_count = tile_info->tile_x * tile_info->tile_y;
    
    _tiles = (TerrainTile*)SysRealloc(_tiles, sizeof(TerrainTile) * tile_count);
    
    for (u32 i = 0; i < tile_count; ++i)
    {
        _tiles[i].GenMesh(command_list, tile_info->meshing_strategy, 
                          tile_info->vertex_x, 
                          tile_info->vertex_y, 
                          tile_info->texture_tiling, 
                          tile_info->scale);
        
        r32 pos_x = (r32)(i % tile_info->tile_x);
        r32 pos_z = (r32)(i / tile_info->tile_x);
        
        // @HACK:
        // A grid runs in object space from -0.5, 0.5
        // However the step approach has an actual domain of
        // of -0.5, (0.5 - step_rate). The following two lines
        // adjust for this.
        //
        // NOTE(Dustin): This might not work w/ meshing algorithms
        // other than triangle strip!
        
        pos_x -= pos_x * (1.0f / tile_info->vertex_x);
        pos_z -= pos_z * (1.0f / tile_info->vertex_y);
        
        _tiles[i].SetModelMatrix({ (pos_x), (pos_z) }, tile_info->scale);
    }
    
    _tile_info = *tile_info;
}

// @param command_list: command list to record commands into
// @param proj_view:    Porjection - View Matrix
void 
Terrain::Render(CommandList *command_list, m4 proj_view, TEXTURE_ID heightmap)
{
    if (_wireframe_mode)
        command_list->SetPipelineState(_pso_wireframe._handle);
    else
        command_list->SetPipelineState(_pso_solid._handle);
    
    command_list->SetGraphicsRootSignature(&_root_signature);
    
    u32 tile_count = _tile_info.tile_x * _tile_info.tile_y;
    for (u32 i = 0; i < tile_count; ++i)
    {
        // Set root parameters
        m4 mvp = m4_mul(proj_view, _tiles[i]._model);
        command_list->SetGraphics32BitConstants(terrain::MatrixCB, &mvp);
        
        //tile[i]._heightmap_texture;
        // TODO(Dustin): Actually set the hightmap texture
        command_list->SetShaderResourceView(terrain::HeightmapTexture, 0, heightmap);
        
        command_list->SetVertexBuffer(0, &_tiles[i]._vbuffer);
        command_list->SetIndexBuffer(&_tiles[i]._ibuffer);
        // TODO(Dustin): Set topology based on the meshing strategy
        command_list->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        command_list->DrawIndexedInstanced((UINT)_tiles[i]._ibuffer._count);
    }
}

void
TerrainTile::Free()
{
    _vbuffer.Free();
    _ibuffer.Free();
}

// @param pos:   x,z position on the terrain grid
// @param scale: x,z scale for the tile
void 
TerrainTile::SetModelMatrix(v2 pos, r32 scale)
{
    m4 transform = m4_translate({ pos.x, 0, pos.y });
    //m4 scale_mat = m4_scale(scale, scale, scale);
    m4 scale_mat = m4_scale(scale, 1.0f, scale);
    // TODO(Dustin): Scale matrix
    _model = m4_mul(scale_mat, transform);
}

// @param use_as_texture: a flag to determine if the hightmap is sampled in the shader
//  or in is the height embedded into the vertex.
void 
TerrainTile::AttachTexture(TEXTURE_ID heightmap_texture, bool use_as_texture)
{
}

void 
TerrainTile::GenMesh(CommandList *command_list, TerrainMeshType meshing_strategy, 
                     u32 width, u32 height, r32 tiling, r32 scale)
{
    
    terrain::TerrainGenInfo info{};
    info.width = width;
    info.height = height;
    info.tiling = tiling;
    
    if (TerrainMeshType::Standard == meshing_strategy)
        terrain::GenerateSimpleGrid(&_vbuffer, &_ibuffer, command_list, &info);
    else if (TerrainMeshType::TriangleStrip == meshing_strategy)
        terrain::GenerateTriangleStripGrid(&_vbuffer, &_ibuffer, command_list, &info);
    else if (TerrainMeshType::Pizza == meshing_strategy)
        assert(false && "Not implemented");
    else if (TerrainMeshType::LowPoly == meshing_strategy)
        assert(false && "Not implemented");
    else if (TerrainMeshType::TIN == meshing_strategy)
        assert(false && "Not implemented");
    
}

void 
TerrainTile::Render(CommandList *command_list)
{
}

//-------------------------------------------------------------------------------------------------
// Mesh Generation

// @param vtx_buffer: (output) final vertex buffer
// @param idx_buffer: (output) final index buffer
static void
terrain::GenerateSimpleGrid(VertexBuffer *vtx_buffer, IndexBuffer *idx_buffer,
                            CommandList *command_list, TerrainGenInfo *gen_info)
{
    assert(false && "Not implemented");
}

// @param vtx_buffer: (output) final vertex buffer
// @param idx_buffer: (output) final index buffer
static void 
terrain::GenerateTriangleStripGrid(VertexBuffer *vtx_buffer, IndexBuffer *idx_buffer,
                                   CommandList *command_list, TerrainGenInfo *gen_info)
{
    const r32 min_x = -0.5f;
    const r32 max_x =  0.5f;
    
    const r32 min_z = -0.5f;
    const r32 max_z =  0.5f;
    
    const r32 step_x = (r32)(max_x - min_x) / (r32)gen_info->width;
    const r32 step_z = (r32)(max_z - min_z) / (r32)gen_info->height;
    
    u32 vertex_count = gen_info->width * gen_info->height * 3;
    u32 index_count = (gen_info->width * gen_info->height) + (gen_info->width - 1) * (gen_info->height - 2);
    
#if 0
    
    TerrainVertex *vertices = (TerrainVertex*)SysAlloc(sizeof(TerrainVertex) * vertex_count);
    u32 *indices = (u32*)SysAlloc(sizeof(u32) * index_count);
    
#else
    
    u64 vertex_size = sizeof(TerrainVertex) * vertex_count;
    u64 index_size = sizeof(u32) * index_count;
    
    void *terrain_data_ptr = PlatformVirtualAlloc(vertex_size + index_size);
    TerrainVertex *vertices = (TerrainVertex*)((char*)terrain_data_ptr);
    u32 *indices = (u32*)((char*)terrain_data_ptr + vertex_size);
    
#endif
    
    
    // Generate the grid information
    for (u32 r = 0; r < gen_info->height; ++r)
    {
        u32 base = r * gen_info->width;
        for (u32 c = 0; c < gen_info->width; ++c)
        {
            // Calculate the UVs
            
            r32 uvx = gen_info->tiling * (c / (r32)gen_info->width);
            r32 uvz = gen_info->tiling * (r / (r32)gen_info->height);
            
            vertices[base + c].uvs.x = (r32)(uvx - (i32)uvx);
            vertices[base + c].uvs.y = (r32)(uvz - (i32)uvz);
            
            // Calculate the Position
            
            vertices[base + c].pos.x = min_x + (r32)c * step_x;
            vertices[base + c].pos.y = min_z + (r32)r * step_z;
            
            // Zero Normals for now...
            
            vertices[base + c].norms = V3_ZERO;
        }
    }
    
    // Create the indices list
    u32 i = 0;
    for (u32 r = 0; r < gen_info->height - 1; r++)
    {
        if ((r & 1) == 0)
        { // even rows
            for (u32 c = 0; c < gen_info->width; c++)
            {
                indices[i++] = (r + 0) * gen_info->width + c;
                indices[i++] = (r + 1) * gen_info->width + c;
            }
        }
        else
        {
            for (u32 c = gen_info->width - 1; c > 0; c--)
            {
                indices[i++] = c + (r + 1) * gen_info->width;
                indices[i++] = c - 1 + (r + 0) * gen_info->width;
            }
        }
        
        if ((gen_info->height & 1) && gen_info->height > 2)
        {
            indices[i++] = (gen_info->height - 1) * gen_info->width;
        }
    }
    
    command_list->CopyVertexBuffer(vtx_buffer, vertex_count, sizeof(TerrainVertex), vertices);
    command_list->CopyIndexBuffer(idx_buffer, index_count, sizeof(u32), indices);
    
#if 0
    
    SysFree(vertices);
    SysFree(indices);
    
#else
    
    PlatformVirtualFree(terrain_data_ptr);
    
#endif
    
}

static void 
terrain::GenerateLowPolyGrid()
{
}

static void
terrain::GeneratePizzaGrid()
{
}