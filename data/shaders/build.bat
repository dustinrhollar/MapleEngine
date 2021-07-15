@echo off

:: Core Shaders

fxc /nologo /Od /Zi /T vs_5_1 /FoImGuiVertex.cso             ImGuiVertex.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoImGuiPixel.cso              ImGuiPixel.hlsl

fxc /nologo /Od /Zi /T cs_5_1 /FoGenerateMips_CS.cso         GenerateMips_CS.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoTerrainVertex.cso           TerrainVertex.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoTerrainPixel.cso            TerrainPixel.hlsl

:: Noise algorithms

fxc /nologo /Od /Zi /T cs_5_1 /FoPerlinNoise.cso             NoiseFunctions/PerlinNoise.hlsl
fxc /nologo /Od /Zi /T cs_5_1 /FoSimplexNoise.cso            NoiseFunctions/SimplexNoise.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoHeightmapDownsample_Vtx.cso HeightmapDownsample_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoHeightmapDownsample_Pxl.cso HeightmapDownsample_Pxl.hlsl

:: Experimental Shaders

fxc /nologo /Od /Zi /T vs_5_1 /FoCube_Vtx.cso                Experimental/Cube_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoCube_Pxl.cso                Experimental/Cube_Pxl.hlsl
