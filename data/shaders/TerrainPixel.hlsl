
struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

float4 main( PixelShaderInput IN ) : SV_Target
{
//    return float4(1.0f, 0.0f, 0.0f, 1.0f);
    return float4(IN.TexCoord.x, IN.TexCoord.y, 0.0f, 1.0f);
}