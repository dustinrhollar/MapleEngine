
struct Cube
{
    VertexBuffer vbuffer;
    IndexBuffer  ibuffer;
};

static Cube CreateCube(CommandList *command_list, r32 size = 1.0f, bool reverse_winding = false, bool invert_normals = false);
static void RenderCube(CommandList *command_list, Cube *cube);
static void FreeCube(Cube *cube);

static Cube 
CreateCube(CommandList *command_list, r32 size, bool reverse_winding, bool invert_normals)
{
    Cube result = {};
    
    // 8 edges of cube.
    v3 p[8] = { 
        {  size, size, -size }, {  size, size,  size }, {  size, -size,  size }, {  size, -size, -size },
        { -size, size,  size }, { -size, size, -size }, { -size, -size, -size }, { -size, -size,  size } 
    };
    // 6 face normals
    v3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    // 4 unique texture coordinates
    v2 t[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    
    // Indices for the vertex positions.
    u16 i[24] = {
        0, 1, 2, 3,  // +X
        4, 5, 6, 7,  // -X
        4, 1, 0, 5,  // +Y
        2, 7, 6, 3,  // -Y
        1, 4, 7, 2,  // +Z
        5, 0, 3, 6   // -Z
    };
    
    VertexPositionNormalTex vertices[24];
    u16 indices[36];
    
    if (invert_normals)
    {
        for (u16 f = 0; f < 6; ++f )  // For each face of the cube.
        {
            n[f] = v3_mulf(n[f], -1.0f);
        }
    }
    
    u16 vidx = 0;
    u16 iidx = 0;
    for (u16 f = 0; f < 6; ++f )  // For each face of the cube.
    {
        // Four vertices per face.
        vertices[vidx++] = { p[i[f * 4 + 0]], n[f], t[0] };
        vertices[vidx++] = { p[i[f * 4 + 1]], n[f], t[1] };
        vertices[vidx++] = { p[i[f * 4 + 2]], n[f], t[2] };
        vertices[vidx++] = { p[i[f * 4 + 3]], n[f], t[3] };
        
        // First triangle.
        indices[iidx++] =  f * 4 + 0;
        indices[iidx++] =  f * 4 + 1;
        indices[iidx++] =  f * 4 + 2;
        
        // Second triangle.
        indices[iidx++] =  f * 4 + 2;
        indices[iidx++] =  f * 4 + 3;
        indices[iidx++] =  f * 4 + 0;
    }
    
    if (reverse_winding)
    {
        ReverseWinding(indices, _countof(indices), vertices, _countof(vertices));
    }
    
    command_list->CopyVertexBuffer(&result.vbuffer, _countof(vertices), sizeof(VertexPositionNormalTex), vertices);
    command_list->CopyIndexBuffer(&result.ibuffer, _countof(indices), sizeof(u16), indices);
    
    return result;
}

static void 
RenderCube(CommandList *command_list, Cube *cube)
{
    command_list->SetVertexBuffer(0, &cube->vbuffer);
    command_list->SetIndexBuffer(&cube->ibuffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->DrawIndexedInstanced((UINT)cube->ibuffer._count);
}

static void 
FreeCube(Cube *cube)
{
    cube->vbuffer.Free();
    cube->ibuffer.Free();
}