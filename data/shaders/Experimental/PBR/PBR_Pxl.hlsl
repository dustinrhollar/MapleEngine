
#define PI 3.14159265359

#define ALBEDO_BIT    (1<<0)
#define NORMAL_BIT    (1<<1)
#define METALLIC_BIT  (1<<2)
#define ROUGHNESS_BIT (1<<3)
#define AO_BIT        (1<<4)

struct Material
{
    float4 ambient;
    //------------------------ 16 byte boundary
    float4 albedo;
    //------------------------ 16 bytes boundary
    float4 specular;
    //------------------------ 16 bytes boundary
    float  metallic;
    float  roughness;
    float  ao;
    float  f0; // surface reflection at zero incidence (IoR)
    //------------------------ 16 bytes boundary
    // NOTE(Dustin): As I integrate more texture maps into the shader,
    // could use a bitmask instead of a series of boolean values 
    uint   has_texture; 
    float  specular_power; // could remove?
    float2 pad1;
    //------------------------ 16 bytes boundary
    //------------------------ Total: 80 bytes
};

struct LightProperties
{
    float3 CameraPos;
    float  pad0;
    uint   NumPointLights;
    uint   NumSpotLights;
    //float2 pad0;
    //float  pad1;
};

struct LightResult
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
};

struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS   : NORMAL;
	float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_POSITION;
};

struct PointLight
{
    float4 position_ws;
    //------------------------ 16 byte boundary
    float4 color;
    //------------------------ 16 byte boundary
    //------------------------ Total: 32 bytes
};

ConstantBuffer<Material> Material_CB         : register(b0, space1);
ConstantBuffer<LightProperties> LightProp_CB : register(b1, space0);
//ConstantBuffer<DirectionalLight> DirLight_CB : register(b2, space0);

StructuredBuffer<PointLight> PointLights     : register(t0, space0);
//StructuredBuffer<Spotlight> Spotlights       : register(t1, space0);
Texture2D AlbedoTexture                      : register(t2, space0);
Texture2D NormalTexture                      : register(t3, space0);
Texture2D MetallicTexture                    : register(t4, space0);
Texture2D RoughnessTexture                   : register(t5, space0);
Texture2D AOTexture                          : register(t6, space0);


SamplerState LinearRepeatSampler             : register(s0, space0);

float Distribution_GGX(float3 N, float3 H, float roughness)
{
    // based on observations from Disney, lighting looks more correct squaring
    // the roughness term in both geometry & ND functions
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001f);
}

float3 Fresnel_Schlick(float cos_theta, float3 F0)
{
    return max(F0 + (1.0f - F0) * pow(max(1.0f - cos_theta, 0.0f), 5.0f), 0.0f);
}

float Geometry_SchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float Geometry_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);

    float ggx1 = Geometry_SchlickGGX(NdotV, roughness);
    float ggx2 = Geometry_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float4 CalculateLighting(float3 N, float3 P, float2 tex_coord)
{
    float3 albedo = (Material_CB.has_texture & ALBEDO_BIT)                  ? 
        pow(AlbedoTexture.Sample(LinearRepeatSampler, tex_coord).rgb, 2.2f) : 
        Material_CB.albedo.rgb;

    float  metallic = (Material_CB.has_texture & METALLIC_BIT)     ? 
        MetallicTexture.Sample(LinearRepeatSampler, tex_coord).r   : 
        0.00f;
    
    float  roughness = (Material_CB.has_texture & ROUGHNESS_BIT)  ? 
        RoughnessTexture.Sample(LinearRepeatSampler, tex_coord).r :
        Material_CB.roughness;
    
    float  ao = (Material_CB.has_texture & AO_BIT)         ? 
        AOTexture.Sample(LinearRepeatSampler, tex_coord).r :
        0.0f;

    float3 V = normalize(LightProp_CB.CameraPos - P);

    // Setup fresnel terms
    float3 F0 = float3(Material_CB.f0,Material_CB.f0,Material_CB.f0);
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0,0,0);
    for (uint i = 0; i < LightProp_CB.NumPointLights; ++i)
    {
        float3 diff = PointLights[i].position_ws.xyz - P;

        float3 L = normalize(diff);
        float3 H = normalize(V + L);

        float distance = length(diff);
        float attentuation = 1.0f / (distance * distance);
        float3 radiance = PointLights[i].color.rgb * attentuation;

        float D = Distribution_GGX(N, H, roughness);
        float3 F = Fresnel_Schlick(clamp(dot(H, V), 0.0f, 1.0f), F0);
        float G = Geometry_Smith(N, V, L, roughness);

        float3 num = D*F*G;
        float denom = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
        float3 specular = num / max(denom, 0.001f);

        float3 ks = F; // specular coefficient
        float3 kd = float3(1.0f, 1.0f, 1.0f) - ks; // diffuse coefficient
        kd *= 1.0f - metallic;

        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kd * albedo / PI + specular) * radiance * NdotL;
    }

    float3 ambient = 0.03f * albedo * ao;
    float3 color = ambient + Lo;

    // Gamma Correct
    float gamma = 1.0f/2.2f;

    color = color / (color + 1.0f);
    color = pow(color, gamma);

    return float4(color, 1.0f);
}

float3 GetNormalFromTexture(float3 Normal, float2 TexCoord, float3 Position_WS)
{
    float3 tangent_normal = NormalTexture.Sample(LinearRepeatSampler, TexCoord).xyz * 2.0f - 1.0f;

    float3 Q1 = ddx(Position_WS);
    float3 Q2 = ddy(Position_WS);
    float2 st1 = ddx(TexCoord);
    float2 st2 = ddy(TexCoord);

    float3 N = normalize(Normal);
    float3 T = normalize(Q1 * st2.y - Q2*st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(TBN, tangent_normal));
}

float4 main( PixelShaderInput IN ) : SV_Target
{
    float3 N = (Material_CB.has_texture & NORMAL_BIT)                     ? 
        GetNormalFromTexture(IN.NormalWS, IN.TexCoord, IN.PositionWS.xyz) : 
        normalize(IN.NormalWS);
    return CalculateLighting(N, IN.PositionWS.xyz, IN.TexCoord);  
}