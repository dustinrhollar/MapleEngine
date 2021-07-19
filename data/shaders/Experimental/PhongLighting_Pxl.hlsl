#include "Lights.hlsl"

struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
	float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_POSITION;
};

ConstantBuffer<Material> Material_CB         : register(b0, space1);
ConstantBuffer<LightProperties> LightProp_CB : register(b1, space0);
ConstantBuffer<DirectionalLight> DirLight_CB : register(b2, space0);

StructuredBuffer<PointLight> PointLights     : register(t0, space0);
StructuredBuffer<Spotlight> Spotlights       : register(t1, space0);
Texture2D DiffuseTexture                     : register(t2, space0);

SamplerState LinearRepeatSampler             : register(s0, space0);

// L : light direction
// N : normal (in viewspace)
// V : view direction (in viewspace)
// P : fragment position (in viewspace) 

float CalculateDiffuse(float3 L, float3 N)
{
    return max(dot(N, L), 0.0);
}

float CalculateSpecular(float3 L, float3 N, float3 V)
{
    if (LightProp_CB.UseBlinnPhong)
    {
        float3 H = normalize(L + V);
        float HdotN = max(dot(V, H), 0.0f);
        return pow(HdotN, Material_CB.specular_power);
    }
    else
    {
        float3 R = reflect(-L, N);
        float RdotV = max(dot(V, R), 0.0f);
        return pow(RdotV, Material_CB.specular_power);
    }
}

float Attentuation(float constant, float lin, float quadratic, float distance)
{
    if (LightProp_CB.HasGammaCorrection)
    {
        return 1.0 / (distance * distance);
    }
    else
    {
        return 1.0f / (constant + (lin * distance) + (quadratic * distance * distance));
    }
}

LightResult CalculateDirectionalLighting(DirectionalLight light, float3 N, float3 V)
{
    float3 L = normalize(-light.direction_vs.xyz);

    LightResult result;
    result.diffuse  = light.color * CalculateDiffuse(L, N);
    result.specular = light.color * CalculateSpecular(L, N, V);
    return result;
}

LightResult CalculatePointLight(PointLight light, float3 N, float3 P, float3 V)
{
    float3 Lunorm = light.position_vs.xyz - P;

    float3 L = normalize(Lunorm);
    float d = length(Lunorm);

    float attentuation = Attentuation(light.constant, light.lin, light.quadratic, d);

    LightResult result;
    result.diffuse  = light.color * attentuation * CalculateDiffuse(L, N)     * 1.0f;
    result.specular = light.color * attentuation * CalculateSpecular(L, N, V) * 1.0f;
    return result;
}

LightResult CalculateSpotLight(Spotlight light, float3 N, float3 P, float3 V)
{
    float3 Lunorm = normalize(light.position_vs.xyz - P);

    float3 L = normalize(Lunorm);
    float d = length(Lunorm);
    
    float attentuation = Attentuation(light.constant, light.lin, light.quadratic, d);

    // Spotlight intensity
    float theta = dot(L, normalize(-light.direction_vs.xyz));

    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    LightResult result;
    result.diffuse  = light.color * attentuation * CalculateDiffuse(L, N);
    result.specular = light.color * attentuation * intensity * CalculateSpecular(L, N, V);
    return result;
}

LightResult CalculateLighting(float3 P, float3 N)
{
    uint i;
    
    // Do lighting in View space so that we do not have to pass Camera Position to the shaders.
    float3 V = normalize(-P);

    // Calculate Directional Lighting
    LightResult final_result;
    if (LightProp_CB.HasDirectionalLight)
        final_result = CalculateDirectionalLighting(DirLight_CB, N, V);
    else
    {
        final_result.diffuse = float4(0,0,0,1);
        final_result.specular = float4(0,0,0,1);
    }

    // Calculate Point Lighting
    for (i = 0; i < LightProp_CB.NumPointLights; ++i)
    {
        LightResult result = CalculatePointLight(PointLights[i], N, P, V);

        final_result.diffuse += result.diffuse;
        final_result.specular += result.specular;
    }

    // Calculate Spot lighting
    for (i = 0; i < LightProp_CB.NumSpotLights; ++i)
    {
        LightResult result = CalculateSpotLight(Spotlights[i], N, P, V);

        final_result.diffuse += result.diffuse;
        final_result.specular += result.specular;
    }

    return final_result;
}

float4 main( PixelShaderInput IN ) : SV_Target
{
    LightResult result = CalculateLighting(IN.PositionVS.xyz, normalize(IN.NormalVS));

    float4 ambient  = Material_CB.ambient;
    float4 diffuse  = Material_CB.diffuse * result.diffuse;
    float4 specular = Material_CB.specular * result.specular;
    float4 tex_color = (Material_CB.has_diffuse) ? DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord) : float4(1,1,1,1);

    float4 color = (ambient + diffuse + specular) * tex_color;
    return float4(color.xyz, 1.0f);
}