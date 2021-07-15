#include "NoiseCommon.hlsl"
#include "NoiseData.hlsl"

static const float3 gradMap[12] = { 
    float3( 1.0f, 1.0f, 0.0f ),float3( -1.0f, 1.0f, 0.0f ),float3( 1.0f, -1.0f, 0.0f ),float3( -1.0f, -1.0f, 0.0f ),
	float3( 1.0f, 0.0f, 1.0f ),float3( -1.0f, 0.0f, 1.0f ),float3( 1.0f, 0.0f, -1.0f ),float3( -1.0f, 0.0f, -1.0f ),
	float3( 0.0f, 1.0f, 1.0f ),float3( 0.0f, -1.0f, 1.0f ),float3( 0.0f, 1.0f, -1.0f ),float3( 0.0f, -1.0f, -1.0f )
};

// Random value for simplex noise [0, 255]
uint calcPerm(int p)
{
	return (uint)(hash(p));
}

// Random value for simplex noise [0, 11]
uint calcPerm12(int p)
{
	return (uint)(hash(p) % 12);
}

// Simplex noise adapted from Java code by Stefan Gustafson and Peter Eastman
float simplexNoise(float3 pos, float scale, int seed)
{
	float xin = pos.x * scale;
	float yin = pos.y * scale;
	float zin = pos.z * scale;

	// Skewing and unskewing factors for 3 dimensions
	const float F3 = 1.0f / 3.0f;
	const float G3 = 1.0f / 6.0f;

	float n0, n1, n2, n3; // Noise contributions from the four corners

								// Skew the input space to determine which simplex cell we're in
	float s = (xin + yin + zin)*F3; // Very nice and simple skew factor for 3D
	int i = floor(xin + s);
	int j = floor(yin + s);
	int k = floor(zin + s);
	float t = (i + j + k)*G3;
	float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
	float Y0 = j - t;
	float Z0 = k - t;
	float x0 = xin - X0; // The x,y,z distances from the cell origin
	float y0 = yin - Y0;
	float z0 = zin - Z0;

	// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	// Determine which simplex we are in.
	int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
	int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords
	if (x0 >= y0) {
		if (y0 >= z0)
		{
			i1 = 1.0f; j1 = 0.0f; k1 = 0.0f; i2 = 1.0f; j2 = 1.0f; k2 = 0.0f;
		} // X Y Z order
		else if (x0 >= z0) { i1 = 1.0f; j1 = 0.0f; k1 = 0.0f; i2 = 1.0f; j2 = 0.0f; k2 = 1.0f; } // X Z Y order
		else { i1 = 0.0f; j1 = 0.0f; k1 = 1.0f; i2 = 1.0f; j2 = 0.0f; k2 = 1.0f; } // Z X Y order
	}
	else 
	{ // x0<y0
		if (y0 < z0) { i1 = 0.0f; j1 = 0.0f; k1 = 1.0f; i2 = 0.0f; j2 = 1; k2 = 1.0f; } // Z Y X order
		else if (x0 < z0) { i1 = 0.0f; j1 = 1.0f; k1 = 0.0f; i2 = 0.0f; j2 = 1.0f; k2 = 1.0f; } // Y Z X order
		else { i1 = 0.0f; j1 = 1.0f; k1 = 0.0f; i2 = 1.0f; j2 = 1.0f; k2 = 0.0f; } // Y X Z order
	}

	// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	// c = 1/6.
	float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
	float y1 = y0 - j1 + G3;
	float z1 = z0 - k1 + G3;
	float x2 = x0 - i2 + 2.0f*G3; // Offsets for third corner in (x,y,z) coords
	float y2 = y0 - j2 + 2.0f*G3;
	float z2 = z0 - k2 + 2.0f*G3;
	float x3 = x0 - 1.0f + 3.0f*G3; // Offsets for last corner in (x,y,z) coords
	float y3 = y0 - 1.0f + 3.0f*G3;
	float z3 = z0 - 1.0f + 3.0f*G3;

    int gi0 = calcPerm12(seed + (i * 607495) + (j * 359609) + (k * 654846));
    int gi1 = calcPerm12(seed + (i + i1) * 607495 + (j + j1) * 359609 + (k + k1) * 654846);
    int gi2 = calcPerm12(seed + (i + i2) * 607495 + (j + j2) * 359609 + (k + k2) * 654846);
    int gi3 = calcPerm12(seed + (i + 1) * 607495 + (j + 1) * 359609 + (k + 1) * 654846);

	// Calculate the contribution from the four corners
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	if (t0 < 0.0f) n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * dot(gradMap[gi0], float3(x0, y0, z0));
	}
	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	if (t1 < 0.0f) n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * dot(gradMap[gi1], float3(x1, y1, z1));
	}
	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	if (t2 < 0.0f) n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * dot(gradMap[gi2], float3(x2, y2, z2));
	}
	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0.0f) n3 = 0.0f;
	else {
		t3 *= t3;
		n3 = t3 * t3 * dot(gradMap[gi3], float3(x3, y3, z3));
	}

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to stay just inside [-1,1]
	return 32.0f*(n0 + n1 + n2 + n3);
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
		float val = saturate((simplexNoise(float3(pos_x * scale + 32240.7922f, pos_y * scale + 835622.882f, pos_z * scale + 824.371968f), 1.0f, seed) * 0.3f + 0.5f)) * amp;
    	acc -= val;

        scale_local *= lacunarity;
        amp *= decay;
    }

	if (acc < threshold)
		heightmap[IN.DispatchThreadID.xy] = 0.0f;	
	else
		heightmap[IN.DispatchThreadID.xy] = acc;
}