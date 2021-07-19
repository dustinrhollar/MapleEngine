
struct Material
{
    float4 ambient;
    //------------------------ 16 byte boundary
    float4 diffuse;
    //------------------------ 16 bytes boundary
    float4 specular;
    //------------------------ 16 bytes boundary
    // NOTE(Dustin): As I integrate more texture maps into the shader,
    // could use a bitmask instead of a series of boolean values 
    bool   has_diffuse; 
    float  specular_power;
    float2 pad0;
    //------------------------ 16 bytes boundary
    //------------------------ Total: 64 bytes
};

struct LightProperties
{
    bool   UseBlinnPhong;
    bool   HasDirectionalLight;
    bool   HasGammaCorrection;
    uint   NumPointLights;
    uint   NumSpotLights;
    //float3 CameraPos;
};

struct DirectionalLight
{
    float4 direction_ws;
    //------------------------ 16 byte boundary
    float4 direction_vs;
    //------------------------ 16 byte boundary
    float4 color;
    //------------------------ 16 byte boundary
    //------------------------ Total: 48 bytes
};

struct PointLight
{
    float4 position_ws;
    //------------------------ 16 byte boundary
    float4 position_vs;
    //------------------------ 16 byte boundary
    float4 color;
    //------------------------ 16 byte boundary
    float  constant;
    float  lin;
    float  quadratic;
    float  pad0;
    //------------------------ 16 byte boundary
    //------------------------ Total: 64 bytes
};

struct Spotlight
{
    float4 position_ws;
    //------------------------ 16 byte boundary
    float4 position_vs;
    //------------------------ 16 byte boundary
    float4 direction_ws;
    //------------------------ 16 byte boundary
    float4 direction_vs;
    //------------------------ 16 byte boundary
    float4 color;
    //------------------------ 16 byte boundary
    float  cutoff;
    float  outer_cutoff;
    float  constant;
    float  lin;
    //------------------------ 16 byte boundary
    float  quadratic;
    float3 pad0;
    //------------------------ 16 byte boundary
    //------------------------ Total: 112 bytes
};

struct LightResult
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
};