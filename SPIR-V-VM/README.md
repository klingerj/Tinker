This is a WIP project that hasn't seen attention in awhile. It is a virtual machine that can execute a very small subset of the SPIR-V spec. It was inspired by [this project](https://github.com/dfranx/SPIRV-VM) which you can check out.

I wrote a very quick and dirty rasterizer to test the functionality. Here is the result of a very simple SPIR-V pixel shader evaluated using this VM, good old lambertian shading:
![](../Output/TestImages/spirv_output.bmp)
