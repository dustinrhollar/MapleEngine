
#define PI 3.14159265359

struct PixelShaderInput
{
    float3 Position_WS : POSITION;
    float3 TexCoord    : TEXCOORD;
};

TextureCube<float4> EnvironmentMap : register(t0);
SamplerState LinearClampSampler    : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    // The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    float3 N = normalize(IN.Position_WS);
    
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    // tangnt space calculation from origin point
    float3  up = float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sample_delta = 0.025f;
    float nr_samples = 0.0f;

    float phi_samples = 2.0f * PI;
    float theta_samples = 0.5f * PI;

    for (float phi = 0.0f; phi < phi_samples; phi += sample_delta)
    {
        for (float theta = 0.0f; theta < theta_samples; theta += sample_delta)
        {
            // spherical to cartesian (Tangent space)
            float3 tangent_sample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent to world space
            float3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;

            irradiance += EnvironmentMap.Sample(LinearClampSampler, sample_vec).rgb * cos(theta) * sin(theta);
            nr_samples++;
        }
    }

    irradiance = PI * irradiance * (1.0f / float(nr_samples));
    return float4(irradiance, 1.0f);
}