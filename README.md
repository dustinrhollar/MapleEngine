# Maple Engine

## About

Maple engine is an in-development game engine whose focus is on providing a variety of tools for learning graphics and game engines. 

## Compiling

Maple Engine supports Windows with D3D12. This application has the following dependencies that will have to be setup on the user's machine before compiling this application:
1. A version of at least Visual Studio 10 installed in order to use the `cl` command line suite of tools. The setup script for initializing these tools can be found in `scripts/setup_cl.bat`. You will only need to open this file if the path to your Visual Studio install is different than the path in the script. 
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

Rendering
- [x] Base Rendering Architecture
- [x] ImGui Platform & Rendering Layer
- [x] Experimental Window for graphics testing
- [x] Diffuse IBL and Cubemapping support 
- [ ] Shadow mapping support
- [ ] Support DDS File format
- [ ] Pre-compute & save mips and cubemaps as DDS files
- [ ] Specular IBL & GI support 

Procedural System
- [x] Basic Editor supporting Node Graph and Terrain Generation
- [x] Viewport Camera controls
- [x] Triangle Strip mesh generation
- [x] Compute support for Perlin and Simplex noise algorithms
- [ ] Compute support for other noise algorithms (Worley, Turbulence, etc.)
- [ ] Alternative mesh generation approaches (Low poly, Geo Clipping, TIN) 

GUI
- [x] Custom Window Interface
- [x] Content Browser 
- [ ] Integrate Experimental Viewports into the new Window Interface
- [ ] Node Graph system for terrain generation
- [ ] Material editor

Engine
- [ ] Preprocessor for Code Gen (in development)
- [ ] DLLs for Editor, Engine, and projects
- [ ] Rework central allocator for memory management
- [ ] Multithreading (Thread Pool, Fibers)
- [ ] Entity System

Asset Pipeline 
- [x] Win32 File Manager
- [ ] Asset Manager
- [ ] Terrain file format
- [ ] Scene file format + generator
- [ ] glTF 2.0 mesh loading

## Showcase

### Viewport for Procedural Terrain
![Main](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/proc_terrain.PNG)

### Secondary Viewport for Terrain Generation
![Secondary](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/main_viewport.PNG)

### Experimental Viewport for testing new graphics features 

![Experimental](https://github.com/dustinrhollar/MapleTerrain/blob/main/data/showcase/experimental_editor.PNG)
