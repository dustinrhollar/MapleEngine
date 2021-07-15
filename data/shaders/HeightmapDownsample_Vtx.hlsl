
struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

VertexShaderOutput main(uint VertId : SV_VERTEXID)
{
    VertexShaderOutput output;

    output.TexCoord = float2(VertId & 1, VertId>>1);
    output.Position = float4((output.TexCoord.x - 0.5f) * 2.0f,- (output.TexCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
    
    return output;
}