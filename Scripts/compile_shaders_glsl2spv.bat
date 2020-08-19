@echo off
pushd ..\Shaders
if NOT EXIST .\spv mkdir .\spv

rem Vulkan
set VulkanVersion=1.2.141.2
echo Using Vulkan v%VulkanVersion%
echo.
set VulkanPath="C:\VulkanSDK"\%VulkanVersion%

for %%i in (glsl\*.vert) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_glsl_vert.spv
for %%i in (glsl\*.frag) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_glsl_frag.spv

popd
