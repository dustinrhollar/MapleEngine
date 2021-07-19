struct PixelShaderInput
{
    float4 Position : SV_Position;
};

float4 main( PixelShaderInput IN ) : SV_Target
{
    return float4(1.0f,1.0f,1.0f,1.0f);
}