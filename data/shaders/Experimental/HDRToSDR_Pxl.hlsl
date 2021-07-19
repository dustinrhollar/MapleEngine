struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

struct Tonemapper_CB
{
    bool  use_hdr;
    float exposure;
    float gamma;
};

ConstantBuffer<Tonemapper_CB> Tonemapper : register(b0, space0);

Texture2D HDRTexture          : register(t0);
SamplerState LinearRepeatSampler    : register(s0);

float4 GammaCorrect(float4 color)
{
    float one_over_gamma = 1.0f / Tonemapper.gamma;
    return pow(color, float4(one_over_gamma,one_over_gamma,one_over_gamma,1));
}

float4 Reinhard(float4 hdr_color)
{
    return float4(1,1,1,1) - exp(-hdr_color * Tonemapper.exposure);
}

float4 main( PixelShaderInput IN ) : SV_Target
{
    float4 sdr_color = HDRTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    if (Tonemapper.use_hdr)
    {
        sdr_color = Reinhard(sdr_color);
    }

    return float4(GammaCorrect(sdr_color).xyz, 1.0);
}