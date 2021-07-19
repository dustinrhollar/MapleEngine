struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD; 
};

struct VertexShaderOutput
{
    float4 Position   : SV_Position;
};

struct Mat
{
    matrix ModelViewProjection;
};

ConstantBuffer<Mat> Mat_CB : register(b0, space0);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput result;
    result.Position   = mul(Mat_CB.ModelViewProjection, float4(IN.Position, 1.0f));
    return result;
}
