
#include "../MathCommon.hlsl"

struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionVS : POSITION;    // Position in viewspace
    float3 NormalVS   : NORMAL;      // Normals in viewspace
	float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

struct Mat
{
    matrix Model;
    matrix ModelView;
    matrix InverseTransposeModelView;
    matrix ModelViewProjection;
};

ConstantBuffer<Mat> Mat_CB : register(b0, space0);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput result;
    result.Position   = mul(Mat_CB.ModelViewProjection, float4(IN.Position, 1.0f));
    result.PositionVS = mul(Mat_CB.ModelView, float4(IN.Position, 1.0f));
    result.TexCoord   = IN.TexCoord;
    result.NormalVS   = mul((float3x3)Mat_CB.InverseTransposeModelView, IN.Normal);
    return result;
}