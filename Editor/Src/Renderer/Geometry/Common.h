
#ifndef _GEOMETRY_COMMON_H
#define _GEOMETRY_COMMON_H

struct VertexPositionNormalTex
{
    v3 pos;
    v3 norm;
    v2 tex;
};

template<typename V, typename I> void
ReverseWinding(I *indices, u32 index_count, V *vertices, u32 vertex_count)
{
    for (u32 i = 0; i < index_count; i += 3)
    {
        I _x = indices[i];
        I _y = indices[i+2];
        
        indices[i]   = _y;
        indices[i+2] = _x;
    }
    
    for (u32 i = 0; i < vertex_count; ++i)
    {
        vertices[i].tex.x = (1.0f - vertices[i].tex.x);
    }
}

#endif //_COMMON_H
