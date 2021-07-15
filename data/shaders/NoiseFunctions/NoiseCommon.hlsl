
#define EPSILON 0.000000001f

// Hashing function (used for fast on-device pseudorandom numbers for randomness in noise)
unsigned int hash(unsigned int seed)
{
    seed = (seed + 0x7ed55d16) + (seed << 12);
    seed = (seed ^ 0xc761c23c) ^ (seed >> 19);
    seed = (seed + 0x165667b1) + (seed << 5);
    seed = (seed + 0xd3a2646c) ^ (seed << 9);
    seed = (seed + 0xfd7046c5) + (seed << 3);
    seed = (seed ^ 0xb55a4f09) ^ (seed >> 16);

    return seed;
}

// Returns a random integer between [min, max]
int randomIntRange(int min, int max, int seed)
{
    int base = hash(seed);
    base = base % (1 + max - min) + min;

    return base;
}

// Returns a random float between [0, 1]
float randomFloat(unsigned int seed)
{
    unsigned int noiseVal = hash(seed);

    return ((float)noiseVal / (float)0xffffffff);
}

// Clamps val between [min, max]
float clamp(float val, float min, float max)
{
    if (val < 0.0f)
        return 0.0f;
    else if (val > 1.0f)
        return 1.0f;

    return val;
}

// Maps from the signed range [0, 1] to unsigned [-1, 1]
// NOTE: no clamping
float mapToSigned(float input)
{
    return input * 2.0f - 1.0f;
}

// Maps from the unsigned range [-1, 1] to signed [0, 1]
// NOTE: no clamping
float mapToUnsigned(float input)
{
    return input * 0.5f + 0.5f;
}

// Maps from the signed range [0, 1] to unsigned [-1, 1] with clamping
float clampToSigned(float input)
{
    return saturate(input) * 2.0f - 1.0f;
}

// Maps from the unsigned range [-1, 1] to signed [0, 1] with clamping
float clampToUnsigned(float input)
{
    return saturate(input * 0.5f + 0.5f);
}

// Random float for a grid coordinate [-1, 1]
float randomGrid(int x, int y, int z, int seed = 0)
{
    return mapToSigned(randomFloat((unsigned int)(x * 1723.0f + y * 93241.0f + z * 149812.0f + 3824.0f + seed)));
}

// Random unsigned int for a grid coordinate [0, MAXUINT]
unsigned int randomIntGrid(float x, float y, float z, float seed = 0.0f)
{
    return hash((unsigned int)(x * 1723.0f + y * 93241.0f + z * 149812.0f + 3824 + seed));
}

// Random 3D vector as float3 from grid position
float3 vectorNoise(int x, int y, int z)
{
    return float3(randomFloat(x * 8231.0f + y * 34612.0f + z * 11836.0f + 19283.0f) * 2.0f - 1.0f,
                  randomFloat(x * 1171.0f + y * 9234.0f + z * 992903.0f + 1466.0f) * 2.0f - 1.0f,
                  0.0f);
}

// Scale 3D vector by scalar value
float3 scaleVector(float3 v, float factor)
{
    return float3(v.x * factor, v.y * factor, v.z * factor);
}

// Scale 3D vector by nonuniform parameters
float3 nonuniformScaleVector(float3 v, float xf, float yf, float zf)
{
    return float3(v.x * xf, v.y * yf, v.z * zf);
}

// 1D cubic interpolation with four points
float cubic(float p0, float p1, float p2, float p3, float x)
{
    return p1 + 0.5f * x * (p2 - p0 + x * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 + x * (3.0f * (p1 - p2) + p3 - p0)));
}