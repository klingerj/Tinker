#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"

#include <iostream>

extern "C"
GAME_UPDATE(GameUpdate)
{
    std::cout << "Joe" << std::endl;

    uint32 a;
    uint32 b = Tinker::Platform::AtomicGet32(&a);
    
    Tinker::Memory::LinearAllocator<1024, 1> linearAllocator;

    v2f v;

    Tinker::Containers::RingBuffer<v2f, 1024> ringBuffer;

    return 0;
}
