struct Mat
{
    matrix ViewProjectionMatrix;
};

struct VertexShaderInput
{
    float3 Position : POSITION;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexShaderOutput
{
    float3 Position_WS : POSITION;
    float3 TexCoord    : TEXCOORD;
    float4 Position    : SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput IN, uint VertId : SV_VERTEXID)
{
    float4 pos = mul(MatCB.ViewProjectionMatrix, float4(IN.Position, 1.0f));

    VertexShaderOutput OUT;
    OUT.Position_WS = IN.Position;
    OUT.Position    = pos.xyww;
    OUT.TexCoord    = IN.Position;
    return OUT;
}