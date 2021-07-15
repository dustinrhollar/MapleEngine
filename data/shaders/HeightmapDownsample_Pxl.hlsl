struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

Texture2D HeightmapTexture          : register(t0);
SamplerState LinearRepeatSampler    : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    float color = HeightmapTexture.Sample(LinearRepeatSampler, IN.TexCoord).x;
    return float4(color, color, color, 1.0f);
}