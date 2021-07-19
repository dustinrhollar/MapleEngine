#ifndef _LIGHTING_H
#define _LIGHTING_H

struct Mat_CB
{
    m4 Model;
    m4 ModelView;
    m4 TransposeModelView; // TODO(Dustin): Invert this on CPU instead
    m4 ModelViewProjection;
};

struct LightProperties
{
    u32  UseBlinnPhong;
    u32  HasDirectionalLight;
    u32  HasGammaCorrection;
    u32  NumPointLights;
    u32  NumSpotLights;
    //v3   CameraPos;
};

struct DirectionalLight
{
    v4 direction_ws;
    //------------------------ 16 byte boundary
    v4 direction_vs;
    //------------------------ 16 byte boundary
    v4 color;
    //------------------------ 16 byte boundary
    //------------------------ Total: 48 bytes
};

struct PointLight
{
    v4 position_ws;
    //------------------------ 16 byte boundary
    v4 position_vs;
    //------------------------ 16 byte boundary
    v4 color;
    //------------------------ 16 byte boundary
    r32  constant;
    r32  lin;
    r32  quadratic;
    r32  pad0;
    //------------------------ 16 byte boundary
    //------------------------ Total: 64 bytes
};

struct SpotLight
{
    v4  position_ws;
    //------------------------ 16 byte boundary
    v4  position_vs;
    //------------------------ 16 byte boundary
    v4  direction_ws;
    //------------------------ 16 byte boundary
    v4  direction_vs;
    //------------------------ 16 byte boundary
    v4  color;
    //------------------------ 16 byte boundary
    r32 cutoff;
    r32 outer_cutoff;
    r32 constant;
    r32 lin;
    //------------------------ 16 byte boundary
    r32 quadratic;
    v3  pad0;
    //------------------------ 16 byte boundary
    //------------------------ Total: 112 bytes
};

#endif //_LIGHTING_H