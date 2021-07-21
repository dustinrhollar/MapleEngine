@echo off

:: Core Shaders

fxc /nologo /Od /Zi /T vs_5_1 /FoImGuiVertex.cso             ImGuiVertex.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoImGuiPixel.cso              ImGuiPixel.hlsl

fxc /nologo /Od /Zi /T cs_5_1 /FoGenerateMips_CS.cso         GenerateMips_CS.hlsl
fxc /nologo /Od /Zi /T cs_5_1 /FoPanoToCubemap_CS.cso        PanoToCubemap_CS.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoTerrainVertex.cso           TerrainVertex.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoTerrainPixel.cso            TerrainPixel.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoSkybox_Vtx.cso              Skybox_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoSkybox_Pxl.cso              Skybox_Pxl.hlsl

:: Noise algorithms

fxc /nologo /Od /Zi /T cs_5_1 /FoPerlinNoise.cso             NoiseFunctions/PerlinNoise.hlsl
fxc /nologo /Od /Zi /T cs_5_1 /FoSimplexNoise.cso            NoiseFunctions/SimplexNoise.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoHeightmapDownsample_Vtx.cso HeightmapDownsample_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoHeightmapDownsample_Pxl.cso HeightmapDownsample_Pxl.hlsl

:: Experimental Shaders

fxc /nologo /Od /Zi /T vs_5_1 /FoCube_Vtx.cso                Experimental/Cube_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoCube_Pxl.cso                Experimental/Cube_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoEnvMapping_Vtx.cso          Experimental/EnvMapping_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoEnvMapping_Pxl.cso          Experimental/EnvMapping_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoPhongLighting_Vtx.cso       Experimental/PhongLighting_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoPhongLighting_Pxl.cso       Experimental/PhongLighting_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoLightCube_Vtx.cso           Experimental/LightCube_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoLightCube_Pxl.cso           Experimental/LightCube_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoHDRToSDR_Vtx.cso            Experimental/HDRToSDR_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoHDRToSDR_Pxl.cso            Experimental/HDRToSDR_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoHDRLighting_Vtx.cso         Experimental/HDRLighting_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoHDRLighting_Pxl.cso         Experimental/HDRLighting_Pxl.hlsl

fxc /nologo /Od /Zi /T vs_5_1 /FoPBR_Vtx.cso                 Experimental/PBR/PBR_Vtx.hlsl
fxc /nologo /Od /Zi /T ps_5_1 /FoPBR_Pxl.cso                 Experimental/PBR/PBR_Pxl.hlsl