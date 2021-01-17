# Tinker Engine

Successor to the Joe Engine. Everything written from scratch.

#### Rough list of features implemented currently:
* Win32 Platform layer, loads game code as DLL
* Relatively thin graphics layer
* Simple unit testing and benchmarking framework
* Small linear algebra library
* Asset file loading
* Shader hotloading on keypress
* Simple thread-job system with lockless queue
* Custom memory allocators

#### Roadmap of features to come:
* Polish and expand graphics layer
* Memory allocation tracking
* Threading and benchmarking of file loading and parsing
* PNG loading, LZ decompression
* PBR shaders
* Compute shader support
* Post processing, probably render graph
* UI and Font Rendering
* Support more platforms, probably Linux
* Support more graphics APIs, OpenGL/DX12

#### Platforms supported currently:
* Windows 10 / x86

#### Graphics API backends supported currently:
* Vulkan

#### Asset file types supported currently:
* Models
  * OBJ
* Textures
  * BMP

#### Assets used:  
* [CGTrader - Fire Elemental by inalaatzu](https://www.cgtrader.com/free-3d-models/character/fantasy/fire-elemental-29c02a51-2d44-4c4b-9e73-fc5899cd690d)  
* [CGTrader - RTX 3090 by bemtevi3d](https://www.cgtrader.com/free-3d-models/electronics/computer/rtx-3090-graphic-card-3d-model)
