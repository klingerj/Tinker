#include "VectorTypeTests.h"
#include "../TinkerTest.h"

// V2
void Test_v2ConstuctorDefault()
{
    v2f a;
    TINKER_ASSERT(a.x == 0.0f);
    TINKER_ASSERT(a.y == 0.0f);
}

void Test_v2ConstuctorV2()
{
    v2f a;
    a.x = 1.0f;
    a.y = 2.0f;
    v2f b = v2f(a);
    TINKER_ASSERT(a.x == b.x);
    TINKER_ASSERT(a.y == b.y);
}

void Test_v2ConstuctorArray()
{
    v2f a;
    a.x = 1.0f;
    a.y = 1.0f;
    v2f b = v2f(a.m_data);
    TINKER_ASSERT(a.x == b.x);
    TINKER_ASSERT(a.y == b.y);
}

void Test_v2ConstuctorF1()
{
    v2f a;
    a.x = 1.0f;
    a.y = 1.0f;
    v2f b = v2f(a.x);
    TINKER_ASSERT(a.x == b.x);
    TINKER_ASSERT(a.y == b.y);
}

void Test_v2ConstuctorF2()
{
    v2f a;
    a.x = 1.0f;
    a.y = 1.0f;
    v2f b = v2f(a.x, a.y);
    TINKER_ASSERT(a.x == b.x);
    TINKER_ASSERT(a.y == b.y);
}

void Test_v2OpAdd()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    v2f c = a + b;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, 2.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, -2.0f));
}

void Test_v2OpSub()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    v2f c = a - b;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, -4.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, 6.0f));
}

void Test_v2OpMul()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    v2f c = a * b;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, -3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, -8.0f));
}

void Test_v2OpDiv()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    v2f c = a / b;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, -1.0f / 3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, -0.5f));
}

void Test_v2OpAddEq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    a += b;
    TINKER_ASSERT(FLOAT_EQUAL(a.x, 2.0f));
    TINKER_ASSERT(FLOAT_EQUAL(a.y, -2.0f));
}

void Test_v2OpSubEq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    a -= b;
    TINKER_ASSERT(FLOAT_EQUAL(a.x, -4.0f));
    TINKER_ASSERT(FLOAT_EQUAL(a.y, 6.0f));
}

void Test_v2OpMulEq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    a *= b;
    TINKER_ASSERT(FLOAT_EQUAL(a.x, -3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(a.y, -8.0f));
}

void Test_v2OpDivEq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(3.0f, -4.0f);
    a /= b;
    TINKER_ASSERT(FLOAT_EQUAL(a.x, -1.0f / 3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(a.y, -0.5f));
}

void Test_v2Eq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(-1.0f, 2.0f);
    TINKER_ASSERT(a == b);
}

void Test_v2NEq()
{
    v2f a(-1.0f, 2.0f);
    v2f b(-3.0f, 4.0f);
    TINKER_ASSERT(a != b);
}

// M2
void Test_m2ConstuctorDefault()
{
    m2f a;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(a.m_data[i] == 0.0f);
    }
}

void Test_m2ConstuctorM2()
{
    m2f a;
    a.m_data[0] = 1.0f;
    a.m_data[1] = 2.0f;
    a.m_data[2] = 3.0f;
    a.m_data[3] = 4.0f;

    m2f b = m2f(a);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(b.m_data[i] == a.m_data[i]);
    }
}

void Test_m2ConstuctorArray()
{
    m2f a;
    a.m_data[0] = 1.0f;
    a.m_data[1] = 2.0f;
    a.m_data[2] = 3.0f;
    a.m_data[3] = 4.0f;

    m2f b = m2f(a.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(b.m_data[i] == a.m_data[i]);
    }
}

void Test_m2ConstuctorF1()
{
    m2f a;
    a.m_data[0] = 1.0f;
    a.m_data[1] = 2.0f;
    a.m_data[2] = 3.0f;
    a.m_data[3] = 4.0f;

    m2f b = m2f(a.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(b.m_data[i] == a.m_data[i]);
    }
}

void Test_m2Constuctor2V2()
{
    m2f a = m2f(v2f(1.0f, 2.0f), v2f(3.0f, 4.0f));
    TINKER_ASSERT(a.m_data[0] == 1.0f);
    TINKER_ASSERT(a.m_data[1] == 2.0f);
    TINKER_ASSERT(a.m_data[2] == 3.0f);
    TINKER_ASSERT(a.m_data[3] == 4.0f);
}

void Test_m2ConstuctorF4()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    TINKER_ASSERT(a.m_data[0] == 1.0f);
    TINKER_ASSERT(a.m_data[1] == 2.0f);
    TINKER_ASSERT(a.m_data[2] == 3.0f);
    TINKER_ASSERT(a.m_data[3] == 4.0f);
}

void Test_m2OpAdd()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f c = a + b;
    TINKER_ASSERT(c.m_data[0] == 6.0f);
    TINKER_ASSERT(c.m_data[1] == 8.0f);
    TINKER_ASSERT(c.m_data[2] == 10.0f);
    TINKER_ASSERT(c.m_data[3] == 12.0f);
}

void Test_m2OpSub()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f c = a - b;
    TINKER_ASSERT(c.m_data[0] == -4.0f);
    TINKER_ASSERT(c.m_data[1] == -4.0f);
    TINKER_ASSERT(c.m_data[2] == -4.0f);
    TINKER_ASSERT(c.m_data[3] == -4.0f);
}

void Test_m2OpMulM2()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f c = a * b;
    TINKER_ASSERT(c.m_data[0] == 5.0f);
    TINKER_ASSERT(c.m_data[1] == 12.0f);
    TINKER_ASSERT(c.m_data[2] == 21.0f);
    TINKER_ASSERT(c.m_data[3] == 32.0f);
}

void Test_m2OpMulV2()
{
    v2f a = v2f(1.0f, 0.0f);
    m2f b = m2f(0.0f, -1.0f, 1.0f, 0.0f);
    v2f c = b * a;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, 0.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, 1.0f));
    c = b * c;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, -1.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, 0.0f));
    c = b * c;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, 0.0f));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, -1.0f));
    c = b * c;
    TINKER_ASSERT(FLOAT_EQUAL(c.x, a.x));
    TINKER_ASSERT(FLOAT_EQUAL(c.y, a.y));
}

void Test_m2OpDiv()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f c = a / b;
    TINKER_ASSERT(c.m_data[0] == 1.0f / 5.0f);
    TINKER_ASSERT(c.m_data[1] == 1.0f / 3.0f);
    TINKER_ASSERT(c.m_data[2] == 3.0f / 7.0f);
    TINKER_ASSERT(c.m_data[3] == 0.5f);
}

void Test_m2OpAddEq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    a += b;
    TINKER_ASSERT(a.m_data[0] == 6.0f);
    TINKER_ASSERT(a.m_data[1] == 8.0f);
    TINKER_ASSERT(a.m_data[2] == 10.0f);
    TINKER_ASSERT(a.m_data[3] == 12.0f);
}

void Test_m2OpSubEq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    a -= b;
    TINKER_ASSERT(a.m_data[0] == -4.0f);
    TINKER_ASSERT(a.m_data[1] == -4.0f);
    TINKER_ASSERT(a.m_data[2] == -4.0f);
    TINKER_ASSERT(a.m_data[3] == -4.0f);
}

void Test_m2OpMulEq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    a *= b;
    TINKER_ASSERT(a.m_data[0] == 5.0f);
    TINKER_ASSERT(a.m_data[1] == 12.0f);
    TINKER_ASSERT(a.m_data[2] == 21.0f);
    TINKER_ASSERT(a.m_data[3] == 32.0f);
}

void Test_m2OpDivEq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    a /= b;
    TINKER_ASSERT(a.m_data[0] == 1.0f / 5.0f);
    TINKER_ASSERT(a.m_data[1] == 1.0f / 3.0f);
    TINKER_ASSERT(a.m_data[2] == 3.0f / 7.0f);
    TINKER_ASSERT(a.m_data[3] == 0.5f);
}

void Test_m2Eq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    TINKER_ASSERT(a == b);
}

void Test_m2NEq()
{
    m2f a = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f b = m2f(-1.0f, -2.0f, -3.0f, -4.0f);
    TINKER_ASSERT(a != b);
}
