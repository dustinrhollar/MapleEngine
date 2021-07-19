#ifndef _MATERIAL_H
#define _MATERIAL_H

struct Material
{
    v4   ambient;
    //------------------------ 16 byte boundary
    v4   diffuse;
    //------------------------ 16 bytes boundary
    v4   specular;
    //------------------------ 16 bytes boundary
    // NOTE(Dustin): As I integrate more texture maps into the shader,
    // could use a bitmask instead of a series of boolean values 
    bool has_diffuse; 
    r32  specular_power;
    v2   pad0;
    //------------------------ 16 bytes boundary
    //------------------------ Total: 64 bytes
};

#endif //_MATERIAL_H
