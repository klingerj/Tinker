#include "Core/Containers/RingBuffer.h"
#include "TinkerTest.h"

using namespace Tinker;
using namespace Containers;

void Test_RingBufferConstructorDefault()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_TEST_ASSERT(buffer.m_data);
    TINKER_TEST_ASSERT(buffer.m_head == 0);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Capacity() == 1);
    TINKER_TEST_ASSERT(buffer.Size() == 0);
}

void Test_RingBufferEnqueueOne()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_TEST_ASSERT(buffer.Capacity() == 1);

    buffer.Enqueue(1);
    TINKER_TEST_ASSERT(buffer.m_data[0] == 1);
    TINKER_TEST_ASSERT(buffer.m_head == 1);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 1);
}

void Test_RingBufferEnqueueManyOverflow()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_TEST_ASSERT(buffer.Capacity() == 1);

    buffer.Enqueue(1);
    buffer.Enqueue(2);
    TINKER_TEST_ASSERT(buffer.m_data[0] == 2);
    TINKER_TEST_ASSERT(buffer.m_head == 2);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 1); // TODO: what should this be?
}

void Test_RingBufferEnqueueOneDequeueOne()
{
    RingBuffer<uint32, 1> buffer;
    TINKER_TEST_ASSERT(buffer.Capacity() == 1);

    buffer.Enqueue(1);
    TINKER_TEST_ASSERT(buffer.m_data[0] == 1);
    TINKER_TEST_ASSERT(buffer.m_head == 1);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 1);

    uint32 x;
    buffer.Dequeue(&x);
    TINKER_TEST_ASSERT(x == 1);
    TINKER_TEST_ASSERT(buffer.m_head == 1);
    TINKER_TEST_ASSERT(buffer.m_tail == 1);
    TINKER_TEST_ASSERT(buffer.Size() == 0);
}

void Test_RingBufferEnqueueManyDequeueMany()
{
    RingBuffer<uint32, 3> buffer;
    TINKER_TEST_ASSERT(buffer.Capacity() == 4);

    buffer.Enqueue(1);
    TINKER_TEST_ASSERT(buffer.m_data[0] == 1);
    TINKER_TEST_ASSERT(buffer.m_head == 1);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 1);

    buffer.Enqueue(2);
    TINKER_TEST_ASSERT(buffer.m_data[1] == 2);
    TINKER_TEST_ASSERT(buffer.m_head == 2);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 2);

    buffer.Enqueue(3);
    TINKER_TEST_ASSERT(buffer.m_data[2] == 3);
    TINKER_TEST_ASSERT(buffer.m_head == 3);
    TINKER_TEST_ASSERT(buffer.m_tail == 0);
    TINKER_TEST_ASSERT(buffer.Size() == 3);

    uint32 x;
    buffer.Dequeue(&x);
    TINKER_TEST_ASSERT(x == 1);
    TINKER_TEST_ASSERT(buffer.m_head == 3);
    TINKER_TEST_ASSERT(buffer.m_tail == 1);
    TINKER_TEST_ASSERT(buffer.Size() == 2);

    buffer.Dequeue(&x);
    TINKER_TEST_ASSERT(x == 2);
    TINKER_TEST_ASSERT(buffer.m_head == 3);
    TINKER_TEST_ASSERT(buffer.m_tail == 2);
    TINKER_TEST_ASSERT(buffer.Size() == 1);

    buffer.Dequeue(&x);
    TINKER_TEST_ASSERT(x == 3);
    TINKER_TEST_ASSERT(buffer.m_head == 3);
    TINKER_TEST_ASSERT(buffer.m_tail == 3);
    TINKER_TEST_ASSERT(buffer.Size() == 0);
}
