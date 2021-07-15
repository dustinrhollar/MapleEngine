struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexInput
{
    float2 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

Texture2D HeightmapTexture          : register(t0);
SamplerState LinearRepeatSampler    : register(s0);

VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;

#if 1
	float height = HeightmapTexture.SampleLevel(LinearRepeatSampler, IN.TexCoord, 0).x;
    float4 pos = float4(IN.Position.x, height * 5.0f, IN.Position.y, 1.0f);
#else
	float4 pos = float4(IN.Position.x, 0.0f, IN.Position.y, 1.0f);
#endif

    OUT.Position = mul(ModelViewProjectionCB.MVP, pos);
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}