# Tinker Engine

My second game engine project. Made with things learned from Handmade Hero, coworkers, and lessons learned from the older [Joe Engine](https://github.com/klingerj/Joe-Engine). Nearly everything written from scratch, extremely minimal libraries/dependencies. Tinker has superior software architecture, minimal dependencies, compiles faster, and has other debatably cooler features and capabilities than the Joe Engine.

### Platforms supported
| | Windows | Linux |
|-|:-------:|:------:
|Vulkan| ✔️ | :x: |
|DX12  | :x: | :x: |

### Table of Contents
* [Project hierarchy description](#project-hierarchy-description)
* [Feature List](#feature-list)
* [Feature Roadmap](#feature-roadmap)
* [Build Instructions](#build-instructions)
* [Dependencies and Licensing](#dependencies-and-licensing)

### Project Hierarchy Description
* <code>Assets/</code> - art files
* <code>Benchmark/</code> - simple setup for running benchmarked code
* <code>Build/</code> - dir for exe/libs/dlls (generated)
* <code>Core/</code> - core engine, builds as exe
  * <code>DataStructures/</code> - data structures, e.g. vector and hashmap
  * <code>Math/</code> - math types and ops
  * <code>Platform/</code> - platform/threading api layer as well as app main, e.g. Windows
  * <code>Utility/</code> - logging, mem alloc tracking, code block timing
* <code>DebugUI/</code> - debug ui / imgui 
* <code>Game/</code> - game code, builds as dll
* <code>Graphics/</code> - graphics api layer as well as backends, e.g. Vulkan
* <code>Output/</code> - dumping ground for output files, e.g. test images
* <code>Scripts/</code> - build scripts and some helpful project scripts
* <code>Shaders/</code> - all gpu shader code
  * <code>hlsl/</code> - shader source
  * <code>spv/</code> - compiled shader bytecode for vulkan backend
* <code>SPIR-V-VM/</code> - virtual machine for evaluating SPIR-V shaders (WIP)
* <code>Test/</code> - simple setup for running unit tests
* <code>ThirdParty/</code> - external libraries
  * <code>dxc_2022_07_18/</code> - DirectXCompiler release lib
  * <code>imgui-docking/</code> - Imgui lib, docking branch
  * <code>xxHash-0.8.2/</code> - xxHash fast string hashing
  * <code>constexpr-xxh3/</code> - compile-time string hashing (produces values identical xxHash)
* <code>Tools/</code> - code for separate tools
  * <code>ShaderCompiler/</code> - calls into DXC to compile HLSL shaders to SPIR-V (+DXIL eventually) 
* <code>ToolsBin/</code> - built binaries of code from <code>Tools/</code>
* <code>Utils/</code> - random misc, e.g. Natvis files

### Feature List
* Platform layer
  * Win32 backend support 
  * Automatic hotloading of game code dll upon recompilation
  * Simple thread-job system with SPSC lockless queue
* Graphics
  * API-agnostic graphics layer, compiles with desired backend API (e.g. Vulkan)
    * Vulkan backend support
      * Dynamic rendering API feature
    * "Bindless" texture binding
    * Render pass timing with GPU timestamps
  * Builds with game dll, meaning the graphics system is hotloadable
  * Draw instancing 
  * Shader hotloading on keypress
    * Calls directly into DirectXCompiler lib
  * Shader pipeline state objects handled as blend/depth state permutations selected at run-time 
* Core engine features
  * Custom data structures
    * Vector
    * Hash map
    * Ring buffer
  * Custom memory allocators
  * Compile-time as well as run-time evaluated string hashes
* Memory allocation tracking
  * Tracks entire stack trace 
* Simple linear algebra library
  * SIMD matrix multiply 
* Game asset file loading
  * OBJ models, BMP textures
  * "Cooking" of mesh vertex data into binary format
* ImGui debug UI
  * Supports multi-viewport, creates and destroys windows/swap chains on demand
* Simple unit testing and benchmarking frameworks
* WIP features
  * [SPIR-V virtual machine](SPIR-V-VM)
    * tested to evaluate only some very simple SPIR-V shaders
    * written in C99 and builds separately from rest of Tinker project
  * Simple CPU-side raytracing (WIP)

### Feature Roadmap
* More debug tooling and validation
* GPU-driven culling and drawing
* Post processing effects
* DX12 backend implementation
* Async asset streaming

### Build Instructions
You will need to have installed:
* Visual Studio 2019, because the project will currently check for  
<code>C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat</code>  
(<code>Community</code> and <code>Enterprise</code> versions will also work)
* Vulkan SDK version 1.3.216.0 or higher from [LunarG](https://vulkan.lunarg.com/sdk/home#windows)

To Build:
* Run <code>proj_shell.bat</code> in the <code>Scripts/</code> directory. You can do this by double-clicking in the file explorer or from command line.
* In that shell window, run <code>ez-build_release.bat</code>. This will build every Tinker project.
You should now be able to run the game via <code>run_game.bat</code>.  
You can find more detailed info on the various build scripts [here](Scripts/README.md).

### Dependencies and Licensing
* [DirectX Shader Compiler (DXC)](https://github.com/microsoft/DirectXShaderCompiler): HLSL shader compiler [(LLVM Release License)](https://github.com/microsoft/DirectXShaderCompiler/blob/main/LICENSE.TXT)
* [Imgui - docking branch](https://github.com/ocornut/imgui): Debug ui [(MIT license)](ThirdParty/imgui-docking/LICENSE.txt)
* [xxHash](https://github.com/Cyan4973/xxHash): Run-time string hashing [(2-Clause BSD License)](ThirdParty/xxHash-0.8.2/LICENSE)
* [constexpr-xxh3](https://github.com/chys87/constexpr-xxh3): Compile-time string hashing [(2-Clause BSD License)](https://github.com/chys87/constexpr-xxh3/blob/main/LICENSE)
