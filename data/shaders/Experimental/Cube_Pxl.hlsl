struct PixelShaderInput
{
    float4 Color    : COLOR;
	float2 TexCoord : TEXCOORD;
};

//Texture2D DiffuseTexture            : register(t0);
//SamplerState LinearRepeatSampler    : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    // Return gamma corrected color.
    //return pow( abs( IN.Color ), 1.0f / 2.2f );

#if 0
	float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    return texColor;
#else
    return IN.Color;
#endif
}