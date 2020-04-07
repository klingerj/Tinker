#include "../../Source/Containers/RingBuffer.h"
#include "../../Source/System/WorkerThreadPool.h"
#include "../TinkerTest.h"

void Test_RingBufferConstructorDefault()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_ASSERT(buffer.m_data == nullptr);
    TINKER_ASSERT(buffer.m_head == UINT32_MAX);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Capacity() == 1);
    TINKER_ASSERT(buffer.Size() == 0);
}

void Test_RingBufferPushOne()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_ASSERT(buffer.Capacity() == 1);

    buffer.Push(1);
    TINKER_ASSERT(buffer.m_data[0] == 1);
    TINKER_ASSERT(buffer.m_head == 0);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Size() == 1);
}

void Test_RingBufferPushManyOverflow()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_ASSERT(buffer.Capacity() == 1);

    buffer.Push(1);
    buffer.Push(2);
    TINKER_ASSERT(buffer.m_data[0] == 2);
    TINKER_ASSERT(buffer.m_head == 1);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    //uint32 size = buffer.Size();
    TINKER_ASSERT(buffer.Size() == 1); // TODO: what should this be?
}

void Test_RingBufferPushOnePopOne()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_ASSERT(buffer.Capacity() == 1);

    buffer.Push(1);
    TINKER_ASSERT(buffer.m_data[0] == 1);
    TINKER_ASSERT(buffer.m_head == 0);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Size() == 1);

    uint32 x = buffer.Pop();
    TINKER_ASSERT(x == 1);
    TINKER_ASSERT(buffer.m_head == 0);
    TINKER_ASSERT(buffer.m_tail == 0);
    TINKER_ASSERT(buffer.Size() == 0);
}

void Test_RingBufferPushManyPopMany()
{
    RingBuffer<uint32, 3> buffer;
    TINKER_ASSERT(buffer.Capacity() == 4);

    buffer.Push(1);
    TINKER_ASSERT(buffer.m_data[0] == 1);
    TINKER_ASSERT(buffer.m_head == 0);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Size() == 1);

    buffer.Push(2);
    TINKER_ASSERT(buffer.m_data[1] == 2);
    TINKER_ASSERT(buffer.m_head == 1);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Size() == 2);

    buffer.Push(3);
    TINKER_ASSERT(buffer.m_data[2] == 3);
    TINKER_ASSERT(buffer.m_head == 2);
    TINKER_ASSERT(buffer.m_tail == UINT32_MAX);
    TINKER_ASSERT(buffer.Size() == 3);

    uint32 x = buffer.Pop();
    TINKER_ASSERT(x == 1);
    TINKER_ASSERT(buffer.m_head == 2);
    TINKER_ASSERT(buffer.m_tail == 0);
    TINKER_ASSERT(buffer.Size() == 2);

    x = buffer.Pop();
    TINKER_ASSERT(x == 2);
    TINKER_ASSERT(buffer.m_head == 2);
    TINKER_ASSERT(buffer.m_tail == 1);
    TINKER_ASSERT(buffer.Size() == 1);

    x = buffer.Pop();
    TINKER_ASSERT(x == 3);
    TINKER_ASSERT(buffer.m_head == 2);
    TINKER_ASSERT(buffer.m_tail == 2);
    TINKER_ASSERT(buffer.Size() == 0);
}
