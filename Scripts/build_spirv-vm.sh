#!/usr/bin/env sh

flags="-std=c99 -pedantic-errors -Wall -Werror -Wno-unused-variable"
buildDir="../Build"
objDir="$buildDir/obj_SpirvVM"
sourceDir="../SPIR-V-VM"
mkdir -p $buildDir
mkdir -p $objDir
pushd $buildDir > /dev/null

# Tidy
#clang-tidy $sourceDir/VMShader.c $sourceDir/VMState.c $sourceDir/SpirvVM.c $sourceDir/SpirvOps.c $sourceDir/Main.c

# Build
echo ""
echo Building SpirvVM.exe...
clang $flags -c $sourceDir/VMShader.c -o $objDir/VMShader.o
clang $flags -c $sourceDir/VMState.c -o $objDir/VMState.o
clang $flags -c $sourceDir/SpirvVM.c -o $objDir/SpirvVM.o
clang $flags -c $sourceDir/SpirvOps.c -o $objDir/SpirvOps.o
clang $flags -c $sourceDir/GLSL.std.450_Ops.c -o $objDir/GLSL.std.450_Ops.o
clang $flags -o SpirvVM.exe $sourceDir/Main.c $objDir/VMShader.o $objDir/VMState.o $objDir/SpirvVM.o $objDir/SpirvOps.o $objDir/GLSL.std.450_Ops.o
echo Done.

popd > /dev/null
