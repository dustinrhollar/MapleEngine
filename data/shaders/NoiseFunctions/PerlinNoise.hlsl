#include "NoiseCommon.hlsl"
#include "NoiseData.hlsl"

// Fast gradient function for gradient noise
float grad(int hash, float x, float y, float z)
{
	switch (hash & 0xF)
	{
	case 0x0:
		return x + y;
	case 0x1:
		return -x + y;
	case 0x2:
		return x - y;
	case 0x3:
		return -x - y;
	case 0x4:
		return x + z;
	case 0x5:
		return -x + z;
	case 0x6:
		return x - z;
	case 0x7:
		return -x - z;
	case 0x8:
		return y + z;
	case 0x9:
		return -y + z;
	case 0xA:
		return y - z;
	case 0xB:
		return -y - z;
	case 0xC:
		return y + x;
	case 0xD:
		return -y + z;
	case 0xE:
		return y - x;
	case 0xF:
		return -y - z;
	default:
		return 0; // never happens
	}
}

// Ken Perlin's fade function for Perlin noise
float fade(float t)
{
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); // 6t^5 - 15t^4 + 10t^3
}

// Dot product using a float[3] and float parameters
// NOTE: could be cleaned up
float dot(float g[3], float x, float y, float z)
{
	return g[0] * x + g[1] * y + g[2] * z;
}

float perlinNoise(float3 pos, float scale, int seed)
{
	float fseed = (float)seed;

	pos.x = pos.x * scale;
	pos.y = pos.y * scale;
	pos.z = pos.z * scale;

	// zero corner integer position
	float ix = floor(pos.x);
	float iy = floor(pos.y);
	float iz = floor(pos.z);

	// current position within unit cube
	pos.x -= ix;
	pos.y -= iy;
	pos.z -= iz;

	// adjust for fade
	float u = fade(pos.x);
	float v = fade(pos.y);
	float w = fade(pos.z);

	// influence values
	float i000 = grad(randomIntGrid(ix, iy, iz, fseed), pos.x, pos.y, pos.z);
	float i100 = grad(randomIntGrid(ix + 1.0f, iy, iz, fseed), pos.x - 1.0f, pos.y, pos.z);
	float i010 = grad(randomIntGrid(ix, iy + 1.0f, iz, fseed), pos.x, pos.y - 1.0f, pos.z);
	float i110 = grad(randomIntGrid(ix + 1.0f, iy + 1.0f, iz, fseed), pos.x - 1.0f, pos.y - 1.0f, pos.z);
	float i001 = grad(randomIntGrid(ix, iy, iz + 1.0f, fseed), pos.x, pos.y, pos.z - 1.0f);
	float i101 = grad(randomIntGrid(ix + 1.0f, iy, iz + 1.0f, fseed), pos.x - 1.0f, pos.y, pos.z - 1.0f);
	float i011 = grad(randomIntGrid(ix, iy + 1.0f, iz + 1.0f, fseed), pos.x, pos.y - 1.0f, pos.z - 1.0f);
	float i111 = grad(randomIntGrid(ix + 1.0f, iy + 1.0f, iz + 1.0f, fseed), pos.x - 1.0f, pos.y - 1.0f, pos.z - 1.0f);

	// interpolation
	float x00 = lerp(i000, i100, u);
	float x10 = lerp(i010, i110, u);
	float x01 = lerp(i001, i101, u);
	float x11 = lerp(i011, i111, u);

	float y0 = lerp(x00, x10, v);
	float y1 = lerp(x01, x11, v);

	float avg = lerp(y0, y1, w);

	return avg;
}

[RootSignature( NoiseMap_RootSignature )]
[numthreads( BLOCK_SIZE, BLOCK_SIZE, 1 )]
void main(ComputeShaderInput IN)
{
	float acc = 1.0f;
	float amp = 1.0f;
	float scale_local = scale;

	float pos_x = IN.DispatchThreadID.x + 0.5f;
	float pos_y = IN.DispatchThreadID.y + 0.5f;
	float pos_z = IN.DispatchThreadID.z; // Maybe make this 0?

	for (int i = 0; i < octaves; i++)
	{
    	acc *= 1.0f - saturate(0.5f + 0.5f * perlinNoise(float3(pos_x * scale_local, pos_y * scale_local, pos_z * scale_local), 1.0f, seed ^ ((i + 38) * 27389482))) * amp;

        scale_local *= lacunarity;
        amp *= decay;
    }

	if (acc < threshold)
		heightmap[IN.DispatchThreadID.xy] = 0.0f;	
	else
		heightmap[IN.DispatchThreadID.xy] = acc;
}