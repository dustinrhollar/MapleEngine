#include "../MathCommon.hlsl"

struct ModelViewProjection
{
    matrix ViewProj;
    matrix Model;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosNormal
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
};

struct VertexShaderOutput
{
	float3 Normal        : NORMAL;
    float3 WorldPosition : Position;
    float4 Position      : SV_Position;
};

VertexShaderOutput main(VertexPosNormal IN)
{
    matrix trans = transpose(inverse(ModelViewProjectionCB.Model));
    float3 normal = mul(mat4_to_mat3(trans), IN.Normal);
    float3 world_position = mul(ModelViewProjectionCB.Model, float4(IN.Position, 1.0f)).xyz;

    VertexShaderOutput OUT;
    OUT.WorldPosition = world_position;
    OUT.Normal        = normal;
    OUT.Position      = mul(ModelViewProjectionCB.ViewProj, float4(world_position, 1.0f));
    return OUT;
}