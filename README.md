# Maple Terrain

## About

Maple terrain is an in-development terrain editor whose focus is on providing a variety of tools for generating and developing large landscapes. 

## Compiling

Maple Terrain only supports Windows with D3D12. This application has the following dependencies that will have to be setup on the user's machine before compiling this application:
1. A version of at least Visual Studio 10 installed in order to use the `cl` command line suite of tools. The setup script for initializing these tools can be found in `scripts/setup_cl.bat`. You will only need to open this file if the your path to Visual Studio is different than the path in the script. 
2. CMAKE for compiling IMGUI
3. [FXC](https://docs.microsoft.com/en-us/windows/win32/direct3dtools/fxc) for compiling shaders. 

All other dependencies are located in the `Editor/Ext` and are compiled automatically with the build system. 

Open your preferred command terminal, and navigate to the cloned repository.
```c
// Compile IMGUI
cd scripts && build_imgui.bat && cd ..

// Copy data (textures & shaders) to build directory
build_shaders.bat

// Build application
build_editor.bat

// Run the application
run.bat
```

## Active Task List
- [x] Base Rendering Architecture
- [x] ImGui Platform & Rendering Layer
- [x] Basic Editor supporting Node Graph and Terrain Generation
- [x] Viewport Camera controls
- [x] Triangle Strip based mesh generation
- [ ] Alternative mesh generation approaches (Low poly, Geo Clipping, TIN) 
- [x] Compute support for Perlin and Simplex noise algorithms
- [ ] Compute support for other noise algorithms (Worley, Turbulence, etc.)
- [x] Experimental Window for graphics testing
- [ ] PBR support for terrain materials
- [ ] Node Graph system for terrain generation
- [ ] Material editor


## Showcase

### Primary Viewport for Terrain
![Main](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/main_viewport.PNG)

### Secondary Viewport for Terrain Generation
![Secondary](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/node_editor.PNG)

### Experimental Viewport for testing new graphics features 

![Experimental](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/experimental_editor.PNG)
