
struct PixelShaderInput
{
	float3 Normal        : NORMAL;
    float3 WorldPosition : Position;
};

struct Pixel_CB
{
    float3 CameraPos;
};

ConstantBuffer<Pixel_CB> PixelCB : register(b0, space1);

TextureCube<float4> SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    float3 I = normalize(IN.WorldPosition - PixelCB.CameraPos);
    float3 R = reflect(I, normalize(IN.Normal));
    return float4(SkyboxTexture.Sample(SkyboxSampler, R).xyz, 1.0f);
}