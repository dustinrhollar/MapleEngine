#define BLOCK_SIZE 8

#define VIZ_DIMS 256

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

cbuffer NoiseCB : register( b0 )
{
    float  scale;
	int    seed;
	int    octaves;
	float  lacunarity;
	float  decay;
	float  threshold;
};

#define NoiseMap_RootSignature \
    "RootFlags(0), " \
	"RootConstants(b0, num32BitConstants = 8), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

// Full resolution heightmap (R8_UNORM)
RWTexture2D<float> heightmap : register( u0 );