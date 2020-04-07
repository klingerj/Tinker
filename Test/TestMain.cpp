#include "TinkerTest.h"

#include "MathTests/VectorTypeTests.h"
#include "ContainerTests/RingBufferTests.h"

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
    TINKER_TEST("M2 Operator+", Test_m2OpAdd);
    TINKER_TEST("M2 Operator-", Test_m2OpSub);
    TINKER_TEST("M2 Operator* M2", Test_m2OpMulM2);
    TINKER_TEST("M2 Operator* V2", Test_m2OpMulV2);
    TINKER_TEST("M2 Operator/", Test_m2OpDiv);
    TINKER_TEST("M2 Operator+=", Test_m2OpAddEq);
    TINKER_TEST("M2 Operator-=", Test_m2OpSubEq);
    TINKER_TEST("M2 Operator*=", Test_m2OpMulEq);
    TINKER_TEST("M2 Operator/=", Test_m2OpDivEq);
    TINKER_TEST("M2 Operator==", Test_m2Eq);
    TINKER_TEST("M2 Operator!=", Test_m2NEq);

    TINKER_TEST_PRINT_NAME("Ring Buffer");
    TINKER_TEST("Ring Buffer Constructor Default", Test_RingBufferConstructorDefault);
    TINKER_TEST("Ring Buffer Push One", Test_RingBufferPushOne);
    TINKER_TEST("Ring Buffer Push Many Overflow", Test_RingBufferPushManyOverflow);
    TINKER_TEST("Ring Buffer Push One Pop One", Test_RingBufferPushOnePopOne);
    TINKER_TEST("Ring Buffer Push Many Pop Many", Test_RingBufferPushManyPopMany);
}
