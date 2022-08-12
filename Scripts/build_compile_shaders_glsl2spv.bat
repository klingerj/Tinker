@echo off
pushd ..\Shaders
if NOT EXIST .\spv mkdir .\spv

rem Vulkan
set VulkanPath=%VULKAN_SDK%

echo.
echo ***** Compiling GLSL shaders to SPV *****
echo Using Vulkan SDK: %VulkanPath%
echo glslangValidator
echo.

echo Deleting existing .spv files...
pushd .\spv
del *.spv > NUL 2> NUL
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
