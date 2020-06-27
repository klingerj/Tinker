#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"

#include <cstring>

static void Test_Thread_Func()
{
    const char* msg = "I am from a thread.\n";
    Tinker::Platform::Print(msg, strlen(msg));
}

extern "C"
GAME_UPDATE(GameUpdate)
{
    const char* msg = "Joe\n";
    Tinker::Platform::Print(msg, strlen(msg));

    // Test a thread job
    Tinker::Platform::WorkerJob* job = Tinker::Platform::CreateNewThreadJob(Test_Thread_Func);
    platformFuncs->EnqueueWorkerThreadJob(job);
    Tinker::Platform::WaitOnJob(job);

    // Issue a test draw command
    const uint32 numCommands = 2;
    Tinker::Platform::GraphicsCommand testCmd[numCommands];
    uint32 commandsSize = sizeof(Tinker::Platform::GraphicsCommand) * numCommands;

    testCmd[0].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    // Assign invalid handles
    testCmd[0].m_indexBufferHandle = 0xffffffff;
    testCmd[0].m_vertexBufferHandle = 0xffffffff;
    testCmd[0].m_uvBufferHandle = 0xffffffff;

    testCmd[1].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    // Assign invalid handles
    testCmd[1].m_indexBufferHandle = 0xffffffff;
    testCmd[1].m_vertexBufferHandle = 0xffffffff;
    testCmd[1].m_uvBufferHandle = 0xffffffff;
    
    graphicsCommandStream->m_numCommands = numCommands;
    Tinker::Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    memcpy(graphicsCmdBase, testCmd, commandsSize);
    graphicsCmdBase += commandsSize;

    uint32 a;
    uint32 b = Tinker::Platform::AtomicGet32(&a);
    
    Tinker::Memory::LinearAllocator<1024, 1> linearAllocator;

    v2f v;

    Tinker::Containers::RingBuffer<v2f, 1024> ringBuffer;

    return 0;
}
