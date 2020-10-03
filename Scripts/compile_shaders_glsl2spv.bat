@echo off
pushd ..\Shaders
if NOT EXIST .\spv mkdir .\spv

pushd .\spv
del *.spv
popd

rem Vulkan
set VulkanVersion=1.2.141.2
echo Using Vulkan v%VulkanVersion%
echo.
set VulkanPath="C:\VulkanSDK"\%VulkanVersion%

for %%i in (glsl\*.vert) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_vert_glsl.spv
for %%i in (glsl\*.frag) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_frag_glsl.spv

popd
