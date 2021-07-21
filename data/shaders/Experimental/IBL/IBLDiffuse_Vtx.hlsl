struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionWS : POSITION;    // Position in viewspace
    float3 NormalWS   : NORMAL;      // Normals in worldspace
	float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

struct Mat
{
    matrix Model;
    matrix ModelView;
    matrix InverseTransposeModel;
    matrix ModelViewProjection;
};

ConstantBuffer<Mat> Mat_CB : register(b0, space0);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput result;
    result.Position   = mul(Mat_CB.ModelViewProjection, float4(IN.Position, 1.0f));
    result.PositionWS = mul(Mat_CB.Model, float4(IN.Position, 1.0f));
    result.TexCoord   = IN.TexCoord;
    result.NormalWS   = mul((float3x3)Mat_CB.Model, IN.Normal);
    return result;
}