#include "DataStructures/Vector.h"
#include "TinkerTest.h"

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
  TINKER_TEST_ASSERT(vec[0] == 1);
  vec.PushBackRaw(2);
  TINKER_TEST_ASSERT(vec.Data());
  TINKER_TEST_ASSERT(vec.Size() == 2);
  TINKER_TEST_ASSERT(vec.Capacity() == 2);
  TINKER_TEST_ASSERT(vec[1] == 2);
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

void Test_VectorResizeUpFromEmpty()
{
  Vector<uint32> vec;
  vec.Resize(5);
  TINKER_TEST_ASSERT(vec.Size() == 5);
  TINKER_TEST_ASSERT(vec.Capacity() == 5);
  TINKER_TEST_ASSERT(vec[0] == 0);
  TINKER_TEST_ASSERT(vec[1] == 0);
  TINKER_TEST_ASSERT(vec[2] == 0);
  TINKER_TEST_ASSERT(vec[3] == 0);
  TINKER_TEST_ASSERT(vec[4] == 0);
}

void Test_VectorResizeUpFromNonEmpty()
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

  vec.Resize(5);
  TINKER_TEST_ASSERT(vec.Size() == 5);
  TINKER_TEST_ASSERT(vec.Capacity() == 5);
  TINKER_TEST_ASSERT(vec[0] == 1);
  TINKER_TEST_ASSERT(vec[1] == 2);
  TINKER_TEST_ASSERT(vec[2] == 0);
  TINKER_TEST_ASSERT(vec[3] == 0);
  TINKER_TEST_ASSERT(vec[4] == 0);
}

void Test_VectorResizeDownFromNonEmptyToSmaller()
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
  vec.PushBackRaw(5);
  TINKER_TEST_ASSERT(vec.Data());
  TINKER_TEST_ASSERT(vec.Size() == 3);
  TINKER_TEST_ASSERT(vec.Capacity() == 3);
  TINKER_TEST_ASSERT(vec[2] == 5);
  vec.PushBackRaw(4);
  TINKER_TEST_ASSERT(vec.Data());
  TINKER_TEST_ASSERT(vec.Size() == 4);
  TINKER_TEST_ASSERT(vec.Capacity() == 4);
  TINKER_TEST_ASSERT(vec[3] == 4);
  vec.PushBackRaw(3);
  TINKER_TEST_ASSERT(vec.Data());
  TINKER_TEST_ASSERT(vec.Size() == 5);
  TINKER_TEST_ASSERT(vec.Capacity() == 5);
  TINKER_TEST_ASSERT(vec[4] == 3);

  vec.Resize(2);
  TINKER_TEST_ASSERT(vec.Data());
  TINKER_TEST_ASSERT(vec.Size() == 2);
  TINKER_TEST_ASSERT(vec.Capacity() == 5);
  TINKER_TEST_ASSERT(vec[0] == 1);
  TINKER_TEST_ASSERT(vec[1] == 2);
}
