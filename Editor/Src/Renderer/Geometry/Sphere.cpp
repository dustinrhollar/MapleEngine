
struct Sphere
{
    VertexBuffer vbuffer;
    IndexBuffer  ibuffer;
};

static Sphere CreateSphere(CommandList *command_list, r32 radius = 1.0f, u32 tessellation = 3, bool reverse_winding = false);
static void RenderSphere(CommandList *command_list, Sphere *sphere);
static void FreeSphere(Sphere *sphere);

static Sphere 
CreateSphere(CommandList *command_list, r32 radius, u32 tessellation, bool reverse_winding)
{
    Assert(tessellation > 3);
    
    VertexPositionNormalTex *vertices = 0;
    u32 *indices = 0;
    
    u64 verticalSegments   = tessellation;
    u32 horizontalSegments = tessellation;
    //u32 horizontalSegments = tessellation * 2;
    
    // Create rings of vertices at progressively higher latitudes.
    for ( u64 i = 0; i <= verticalSegments; i++ )
    {
#if 0
        r32 v = 1 - (r32)i / verticalSegments;
        r32 latitude = ( i * MM_PI / verticalSegments ) - MM_PIDIV2;
        r32 dy  = sinf(latitude);
        r32 dxz = cosf(latitude);
#endif
        
        // Create a single ring of vertices at this latitude.
        for ( u64 j = 0; j <= horizontalSegments; j++ )
        {
#if 0
            r32 u = (r32)j / horizontalSegments;
            
            r32 longitude = j * MM_2PI / horizontalSegments;
            r32 dx = sinf(longitude);
            r32 dz = cosf(longitude);
            
            dx *= dxz;
            dz *= dxz;
            
            v3 normal = { dx, dy, dz };
            v2 uv0    = { u, v };
            v3 pos    = v3_mulf(normal, radius);
#else
            
            r32 xSegment = (r32)j / (r32)horizontalSegments;
            r32 ySegment = (r32)i / (r32)verticalSegments;
            r32 xPos = cosf(xSegment * 2.0f * MM_PI) * sinf(ySegment * MM_PI);
            r32 yPos = cosf(ySegment * MM_PI);
            r32 zPos = sinf(xSegment * 2.0f * MM_PI) * sinf(ySegment * MM_PI);
            
            v3 normal = { xPos, yPos, zPos };
            v2 uv0    = { xSegment, ySegment };
            v3 pos    = v3_mulf(normal, radius);
            
#endif
            
            VertexPositionNormalTex vertex = {};
            vertex.pos = pos;
            vertex.norm = normal;
            vertex.tex = uv0;
            arrput(vertices, vertex);
        }
    }
    
    // Fill the index buffer with triangles joining each pair of latitude rings.
    u32 stride = horizontalSegments + 1;
    
#if 0
    for (u32 i = 0; i < verticalSegments; i++ )
    {
        for (u32 j = 0; j <= horizontalSegments; j++)
        {
            u32 nextI = i + 1;
            u32 nextJ = ( j + 1 ) % stride;
            
            arrput(indices, i * stride + nextJ);
            arrput(indices, nextI * stride + j);
            arrput(indices, i * stride + j);
            
            arrput(indices, nextI * stride + nextJ);
            arrput(indices, nextI * stride + j);
            arrput(indices, i * stride + nextJ);
        }
    }
#else
    bool oddRow = false;
    for (unsigned int y = 0; y < verticalSegments; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= horizontalSegments; ++x)
            {
                arrput(indices, y       * (horizontalSegments + 1) + x);
                arrput(indices, (y + 1) * (horizontalSegments + 1) + x);
            }
        }
        else
        {
            for (int x = horizontalSegments; x >= 0; --x)
            {
                arrput(indices, (y + 1) * (horizontalSegments + 1) + x);
                arrput(indices, y       * (horizontalSegments + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
#endif
    
    if (reverse_winding)
    {
        ReverseWinding(indices, (u32)arrlen(indices), vertices, (u32)arrlen(vertices));
    }
    
    Sphere sphere = {};
    
    command_list->CopyVertexBuffer(&sphere.vbuffer, arrlen(vertices), sizeof(VertexPositionNormalTex), vertices);
    command_list->CopyIndexBuffer(&sphere.ibuffer, arrlen(indices), sizeof(indices[0]), indices);
    
    arrfree(vertices);
    arrfree(indices);
    
    return sphere;
}

static void 
RenderSphere(CommandList *command_list, Sphere *sphere)
{
    command_list->SetVertexBuffer(0, &sphere->vbuffer);
    command_list->SetIndexBuffer(&sphere->ibuffer);
    command_list->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    command_list->DrawIndexedInstanced((UINT)sphere->ibuffer._count);
}

static void 
FreeSphere(Sphere *sphere)
{
    sphere->vbuffer.Free();
    sphere->ibuffer.Free();
}

