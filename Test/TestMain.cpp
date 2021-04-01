#include "TinkerTest.h"

#include "MathTests/VectorTypeTests.h"
#include "ContainerTests/RingBufferTests.h"
#include "ContainerTests/VectorTests.h"
#include "MemoryTests/AllocatorTests.h"

uint8 g_AssertFailedFlag = 0;

int main()
{
    TINKER_TEST_PRINT_HEADER;

    TINKER_TEST_PRINT_NAME("Vec2");
    TINKER_TEST("V2 Constructor Default", Test_v2ConstructorDefault);
    TINKER_TEST("V2 Constructor V2", Test_v2ConstructorV2);
    TINKER_TEST("V2 Constructor Array", Test_v2ConstructorArray);
    TINKER_TEST("V2 Constructor One param", Test_v2ConstructorF1);
    TINKER_TEST("V2 Constructor Two param", Test_v2ConstructorF2);
    TINKER_TEST("V2 Operator[]", Test_v2OpBracket);
    TINKER_TEST("V2 Operator+", Test_v2OpAdd);
    TINKER_TEST("V2 Operator-", Test_v2OpSub);
    TINKER_TEST("V2 Operator*", Test_v2OpMul);
    TINKER_TEST("V2 Operator/", Test_v2OpDiv);
    TINKER_TEST("V2 Operator+=", Test_v2OpAddEq);
    TINKER_TEST("V2 Operator-=", Test_v2OpSubEq);
    TINKER_TEST("V2 Operator*=", Test_v2OpMulEq);
    TINKER_TEST("V2 Operator/=", Test_v2OpDivEq);
    TINKER_TEST("V2 Operator==", Test_v2Eq);
    TINKER_TEST("V2 Operator!=", Test_v2NEq);

    TINKER_TEST_PRINT_NAME("Mat2");
    TINKER_TEST("M2 Constructor Default", Test_v2ConstructorDefault);
    TINKER_TEST("M2 Constructor M2", Test_m2ConstructorM2);
    TINKER_TEST("M2 Constructor Array", Test_m2ConstructorArray);
    TINKER_TEST("M2 Constructor One param", Test_m2ConstructorF1);
    TINKER_TEST("M2 Constructor Two param", Test_m2Constructor2V2);
    TINKER_TEST("M2 Constructor Four param", Test_m2ConstructorF4);
    TINKER_TEST("M2 Operator[]", Test_m2OpBracket);
    TINKER_TEST("M2 Operator+", Test_m2OpAdd);
    TINKER_TEST("M2 Operator-", Test_m2OpSub);
    TINKER_TEST("M2 Operator* M2", Test_m2OpMulM2);
    TINKER_TEST("M2 Operator* V2", Test_m2OpMulV2);
    TINKER_TEST("M2 V2 MUL SIMD", Test_m2V2MulSIMD);
    TINKER_TEST("M4 V4 MUL SIMD", Test_m4V4MulSIMD);
    TINKER_TEST("M2 Operator/", Test_m2OpDiv);
    TINKER_TEST("M2 Operator+=", Test_m2OpAddEq);
    TINKER_TEST("M2 Operator-=", Test_m2OpSubEq);
    TINKER_TEST("M2 Operator*=", Test_m2OpMulEq);
    TINKER_TEST("M2 Operator/=", Test_m2OpDivEq);
    TINKER_TEST("M2 Operator==", Test_m2Eq);
    TINKER_TEST("M2 Operator!=", Test_m2NEq);

    TINKER_TEST_PRINT_NAME("Ring Buffer");
    TINKER_TEST("Ring Buffer Constructor Default", Test_RingBufferConstructorDefault);
    TINKER_TEST("Ring Buffer Enqueue One", Test_RingBufferEnqueueOne);
    TINKER_TEST("Ring Buffer Enqueue Many Overflow", Test_RingBufferEnqueueManyOverflow);
    TINKER_TEST("Ring Buffer Enqueue One Dequeue One", Test_RingBufferEnqueueOneDequeueOne);
    TINKER_TEST("Ring Buffer Enqueue Many Dequeue Many", Test_RingBufferEnqueueManyDequeueMany);

    TINKER_TEST_PRINT_NAME("Memory Allocators");
    TINKER_TEST("Linear, 1K 1-byte size, 1-aligned allocs, no allocator alignment", Test_Linear_NoAlignment);
    TINKER_TEST("Linear, 1K 1-byte size, 16-aligned allocs, no allocator alignment", Test_Linear_Alignment);
    TINKER_TEST("Linear, 1K 1-byte size, 1-aligned allocs, no allocator alignment, w/ dealloc", Test_Linear_NoAlignment_WithDealloc);
    
    TINKER_TEST_PRINT_NAME("Vector");
    TINKER_TEST("Vector Constructor Default", Test_VectorConstructorDefault);
    TINKER_TEST("Vector Reserve", Test_VectorReserve);
    TINKER_TEST("Vector Push Back Raw One Ele no Resize", Test_VectorPushBackRawOne);
    TINKER_TEST("Vector Reserve then Push Back Raw One Ele", Test_VectorReservePushBackOne);
    TINKER_TEST("Vector Reserve then Push Back Raw One Then Find", Test_VectorReservePushBackOneFind);
    TINKER_TEST("Vector Push Back Raw Multiple Resize", Test_VectorPushBackMultipleResize);
    TINKER_TEST("Vector Push Back Raw Multiple Clear", Test_VectorPushBackMultipleClear);
    TINKER_TEST("Vector Find Empty", Test_VectorFindEmpty);
}
