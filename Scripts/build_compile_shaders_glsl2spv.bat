@echo off
pushd ..\Shaders
if NOT EXIST .\spv mkdir .\spv

rem Vulkan
set VulkanVersion=1.2.141.2
set VulkanPath="C:\VulkanSDK"\%VulkanVersion%

echo.
echo Compiling GLSL shaders to SPV, using Vulkan SDK v%VulkanVersion% glslangValidator
echo.

echo Deleting existing .spv files...
pushd .\spv
del *.spv
popd
echo Done.
echo.

echo Compiling...
echo.

for %%i in (glsl\*.vert) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_vert_glsl.spv
for %%i in (glsl\*.frag) do %VulkanPath%\Bin\glslangValidator.exe -V %%i -o .\spv\%%~ni_frag_glsl.spv

echo.
echo Done.

popd
