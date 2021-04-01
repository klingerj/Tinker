#include "Core/Containers/Vector.h"
#include "TinkerTest.h"

using namespace Containers;

void Test_VectorConstructorDefault()
{
    Vector<uint32> vec;
    TINKER_TEST_ASSERT(!vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 0);
    TINKER_TEST_ASSERT(vec.Capacity() == 0);
}

void Test_VectorReserve()
{
    Vector<uint32> vec;

    uint32 res = 16;
    vec.Reserve(res);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 0);
    TINKER_TEST_ASSERT(vec.Capacity() == res);
}

void Test_VectorPushBackRawOne()
{
    Vector<uint32> vec;
    vec.PushBackRaw(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    TINKER_TEST_ASSERT(vec[0] == 1);
}

void Test_VectorReservePushBackOne()
{
    Vector<uint32> vec;
    vec.Reserve(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 0);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    vec.PushBackRaw(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    TINKER_TEST_ASSERT(vec[0] == 1);
}

void Test_VectorReservePushBackOneFind()
{
    Vector<uint32> vec;
    vec.Reserve(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 0);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    vec.PushBackRaw(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    TINKER_TEST_ASSERT(vec[0] == 1);
    uint32 i = vec.Find(1);
    TINKER_TEST_ASSERT(vec[i] == 1);
    // Vector is unchanged:
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
}

void Test_VectorPushBackMultipleResize()
{
    Vector<uint32> vec;
    vec.PushBackRaw(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    TINKER_TEST_ASSERT(vec[0] == 1);
    vec.PushBackRaw(2);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 2);
    TINKER_TEST_ASSERT(vec.Capacity() == 2);
    TINKER_TEST_ASSERT(vec[1] == 2);
}

void Test_VectorPushBackMultipleClear()
{
    Vector<uint32> vec;
    vec.PushBackRaw(1);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 1);
    TINKER_TEST_ASSERT(vec.Capacity() == 1);
    vec.PushBackRaw(2);
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 2);
    TINKER_TEST_ASSERT(vec.Capacity() == 2);
    vec.Clear();
    TINKER_TEST_ASSERT(vec.Data());
    TINKER_TEST_ASSERT(vec.Size() == 0);
    TINKER_TEST_ASSERT(vec.Capacity() == 2);
}

void Test_VectorFindEmpty()
{
    Vector<uint32> vec;
    uint32 i = vec.Find(0);
    TINKER_TEST_ASSERT(i == vec.eInvalidIndex);
}

