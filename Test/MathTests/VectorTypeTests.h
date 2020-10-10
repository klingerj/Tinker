#include "../Include/Core/Math/VectorTypes.h"
#include "../TinkerTest.h"

using namespace Tinker;
using namespace Core;
using namespace Math;

// V2
void Test_v2ConstructorDefault()
{
    v2f af;
    TINKER_TEST_ASSERT(af.x == 0.0f);
    TINKER_TEST_ASSERT(af.y == 0.0f);

    v2i ai;
    TINKER_TEST_ASSERT(ai.x == 0);
    TINKER_TEST_ASSERT(ai.y == 0);

    v2ui aui;
    TINKER_TEST_ASSERT(aui.x == 0);
    TINKER_TEST_ASSERT(aui.y == 0);
}

void Test_v2ConstructorV2()
{
    v2f af;
    af.x = 1.0f;
    af.y = 2.0f;
    v2f bf = v2f(af);
    TINKER_TEST_ASSERT(af.x == bf.x);
    TINKER_TEST_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 2;
    v2i bi = v2i(ai);
    TINKER_TEST_ASSERT(ai.x == bi.x);
    TINKER_TEST_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 2;
    v2ui bui = v2ui(aui);
    TINKER_TEST_ASSERT(aui.x == bui.x);
    TINKER_TEST_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorArray()
{
    v2f af;
    af.x = 1.0f;
    af.y = 1.0f;
    v2f bf = v2f(af.m_data);
    TINKER_TEST_ASSERT(af.x == bf.x);
    TINKER_TEST_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 1;
    v2i bi = v2i(ai.m_data);
    TINKER_TEST_ASSERT(ai.x == bi.x);
    TINKER_TEST_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 1;
    v2ui bui = v2ui(aui.m_data);
    TINKER_TEST_ASSERT(aui.x == bui.x);
    TINKER_TEST_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorF1()
{
    v2f af;
    af.x = 1.0f;
    af.y = 1.0f;
    v2f bf = v2f(af.x);
    TINKER_TEST_ASSERT(af.x == bf.x);
    TINKER_TEST_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 1;
    v2i bi = v2i(ai.x);
    TINKER_TEST_ASSERT(ai.x == bi.x);
    TINKER_TEST_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 1;
    v2ui bui = v2ui(aui.x);
    TINKER_TEST_ASSERT(aui.x == bui.x);
    TINKER_TEST_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorF2()
{
    v2f af;
    af.x = 1.0f;
    af.y = 2.0f;
    v2f bf = v2f(af.x, af.y);
    TINKER_TEST_ASSERT(af.x == bf.x);
    TINKER_TEST_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 2;
    v2i bi = v2i(ai.x, ai.y);
    TINKER_TEST_ASSERT(ai.x == bi.x);
    TINKER_TEST_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 2;
    v2ui bui = v2ui(aui.x, aui.y);
    TINKER_TEST_ASSERT(aui.x == bui.x);
    TINKER_TEST_ASSERT(aui.y == bui.y);
}

void Test_v2OpBracket()
{
    v2f af(1.0f, 2.0f);
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[0], af.x));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[0], 1.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[1], af.y));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[1], 2.0f));
    af[0] = 3.0f;
    af[1] = 4.0f;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[0], af.x));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[0], 3.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[1], af.y));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af[1], 4.0f));

    v2i ai(1, 2);
    TINKER_TEST_ASSERT(ai[0] == ai.x);
    TINKER_TEST_ASSERT(ai[0] == 1);
    TINKER_TEST_ASSERT(ai[1] == ai.y);
    TINKER_TEST_ASSERT(ai[1] == 2);
    ai[0] = 3;
    ai[1] = 4;
    TINKER_TEST_ASSERT(ai[0] == ai.x);
    TINKER_TEST_ASSERT(ai[0] == 3);
    TINKER_TEST_ASSERT(ai[1] == ai.y);
    TINKER_TEST_ASSERT(ai[1] == 4);

    v2ui aui(1, 2);
    TINKER_TEST_ASSERT(aui[0] == aui.x);
    TINKER_TEST_ASSERT(aui[0] == 1);
    TINKER_TEST_ASSERT(aui[1] == aui.y);
    TINKER_TEST_ASSERT(aui[1] == 2);
    aui[0] = 3;
    aui[1] = 4;
    TINKER_TEST_ASSERT(aui[0] == aui.x);
    TINKER_TEST_ASSERT(aui[0] == 3);
    TINKER_TEST_ASSERT(aui[1] == aui.y);
    TINKER_TEST_ASSERT(aui[1] == 4);
}

void Test_v2OpAdd()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af + bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, 2.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, -2.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai + bi;
    TINKER_TEST_ASSERT(ci.x == 2);
    TINKER_TEST_ASSERT(ci.y == -2);

    v2ui aui(1, 2);
    v2ui bui(3, 4);
    v2ui cui = aui + bui;
    TINKER_TEST_ASSERT(cui.x == 4);
    TINKER_TEST_ASSERT(cui.y == 6);
}

void Test_v2OpSub()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af - bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, -4.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, 6.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai - bi;
    TINKER_TEST_ASSERT(ci.x == -4);
    TINKER_TEST_ASSERT(ci.y == 6);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui - bui;
    TINKER_TEST_ASSERT(cui.x == 2);
    TINKER_TEST_ASSERT(cui.y == 2);
}

void Test_v2OpMul()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af * bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, -3.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, -8.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai * bi;
    TINKER_TEST_ASSERT(ci.x == -3);
    TINKER_TEST_ASSERT(ci.y == -8);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui * bui;
    TINKER_TEST_ASSERT(cui.x == 3);
    TINKER_TEST_ASSERT(cui.y == 8);
}

void Test_v2OpDiv()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af / bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, -1.0f / 3.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, -0.5f));

    v2i ai(-3, 4);
    v2i bi(1, -2);
    v2i ci = ai / bi;
    TINKER_TEST_ASSERT(ci.x == -3);
    TINKER_TEST_ASSERT(ci.y == -2);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui / bui;
    TINKER_TEST_ASSERT(cui.x == 3);
    TINKER_TEST_ASSERT(cui.y == 2);
}

void Test_v2OpAddEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af += bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.x, 2.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.y, -2.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai += bi;
    TINKER_TEST_ASSERT(ai.x == 2);
    TINKER_TEST_ASSERT(ai.y == -2);

    v2ui aui(1, 2);
    v2ui bui(3, 4);
    aui += bui;
    TINKER_TEST_ASSERT(aui.x == 4);
    TINKER_TEST_ASSERT(aui.y == 6);
}

void Test_v2OpSubEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af -= bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.x, -4.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.y, 6.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai -= bi;
    TINKER_TEST_ASSERT(ai.x == -4);
    TINKER_TEST_ASSERT(ai.y == 6);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui -= bui;
    TINKER_TEST_ASSERT(aui.x == 2);
    TINKER_TEST_ASSERT(aui.y == 2);
}

void Test_v2OpMulEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af *= bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.x, -3.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.y, -8.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai *= bi;
    TINKER_TEST_ASSERT(ai.x == -3);
    TINKER_TEST_ASSERT(ai.y == -8);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui *= bui;
    TINKER_TEST_ASSERT(aui.x == 3);
    TINKER_TEST_ASSERT(aui.y == 8);
}

void Test_v2OpDivEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af /= bf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.x, -1.0f / 3.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(af.y, -0.5f));

    v2i ai(-3, 4);
    v2i bi(1, -2);
    ai /= bi;
    TINKER_TEST_ASSERT(ai.x == -3);
    TINKER_TEST_ASSERT(ai.y == -2);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui /= bui;
    TINKER_TEST_ASSERT(aui.x == 3);
    TINKER_TEST_ASSERT(aui.y == 2);
}

void Test_v2Eq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(-1.0f, 2.0f);
    TINKER_TEST_ASSERT(af == bf);

    v2f ai(-1, 2);
    v2f bi(-1, 2);
    TINKER_TEST_ASSERT(ai == bi);

    v2f aui(1, 2);
    v2f bui(1, 2);
    TINKER_TEST_ASSERT(aui == bui);
}

void Test_v2NEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(0.0f, 0.0f);
    TINKER_TEST_ASSERT(af != bf);

    v2f ai(-1, 2);
    v2f bi(0, 0);
    TINKER_TEST_ASSERT(ai != bi);

    v2f aui(1, 2);
    v2f bui(0, 0);
    TINKER_TEST_ASSERT(aui != bui);
}

// M2
void Test_m2ConstructorDefault()
{
    m2f af;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(af.m_data[i] == 0.0f);
    }

    m2i ai;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(ai.m_data[i] == 0);
    }

    m2ui aui;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(aui.m_data[i] == 0);
    }
}

void Test_m2ConstructorM2()
{
    m2f af;
    af.m_data[0] = 1.0f;
    af.m_data[1] = 2.0f;
    af.m_data[2] = 3.0f;
    af.m_data[3] = 4.0f;

    m2f bf = m2f(af);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[1] = 2;
    ai.m_data[2] = 3;
    ai.m_data[3] = 4;

    m2i bi = m2i(ai);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[1] = 2;
    aui.m_data[2] = 3;
    aui.m_data[3] = 4;

    m2ui bui = m2ui(aui);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bui.m_data[i] == aui.m_data[i]);
    }
}

void Test_m2ConstructorArray()
{
    m2f af;
    af.m_data[0] = 1.0f;
    af.m_data[1] = 2.0f;
    af.m_data[2] = 3.0f;
    af.m_data[3] = 4.0f;

    m2f bf = m2f(af.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[1] = 2;
    ai.m_data[2] = 3;
    ai.m_data[3] = 4;

    m2i bi = m2i(ai.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[1] = 2;
    aui.m_data[2] = 3;
    aui.m_data[3] = 4;

    m2ui bui = m2ui(aui.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bui.m_data[i] == aui.m_data[i]);
    }
}

void Test_m2ConstructorF1()
{
    m2f af;
    af.m_data[0] = 1.0f;
    af.m_data[3] = 1.0f;

    m2f bf = m2f(af.m_data[0]);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[3] = 1;

    m2i bi = m2i(ai.m_data[0]);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[3] = 1;

    m2ui bui = m2ui(aui.m_data[0]);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_TEST_ASSERT(bui.m_data[i] == aui.m_data[i]);
    }
}

void Test_m2Constructor2V2()
{
    m2f af = m2f(v2f(1.0f, 2.0f), v2f(3.0f, 4.0f));
    TINKER_TEST_ASSERT(af.m_data[0] == 1.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == 2.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == 3.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == 4.0f);

    m2i ai = m2i(v2i(1, 2), v2i(3, 4));
    TINKER_TEST_ASSERT(ai.m_data[0] == 1);
    TINKER_TEST_ASSERT(ai.m_data[1] == 2);
    TINKER_TEST_ASSERT(ai.m_data[2] == 3);
    TINKER_TEST_ASSERT(ai.m_data[3] == 4);

    m2ui aui = m2ui(v2ui(1, 2), v2ui(3, 4));
    TINKER_TEST_ASSERT(aui.m_data[0] == 1);
    TINKER_TEST_ASSERT(aui.m_data[1] == 2);
    TINKER_TEST_ASSERT(aui.m_data[2] == 3);
    TINKER_TEST_ASSERT(aui.m_data[3] == 4);
}

void Test_m2ConstructorF4()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    TINKER_TEST_ASSERT(af.m_data[0] == 1.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == 2.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == 3.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == 4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    TINKER_TEST_ASSERT(ai.m_data[0] == 1);
    TINKER_TEST_ASSERT(ai.m_data[1] == 2);
    TINKER_TEST_ASSERT(ai.m_data[2] == 3);
    TINKER_TEST_ASSERT(ai.m_data[3] == 4);

    m2ui aui = m2ui(1, 2, 3, 4);
    TINKER_TEST_ASSERT(aui.m_data[0] == 1);
    TINKER_TEST_ASSERT(aui.m_data[1] == 2);
    TINKER_TEST_ASSERT(aui.m_data[2] == 3);
    TINKER_TEST_ASSERT(aui.m_data[3] == 4);
}

void Test_m2OpBracket()
{
    v2f af = v2f(1.0f, 2.0f);
    v2f bf = v2f(3.0f, 4.0f);
    m2f cf = m2f(af, bf);
    TINKER_TEST_ASSERT(cf[0] == af);
    TINKER_TEST_ASSERT(cf[1] == bf);
    TINKER_TEST_ASSERT(cf[0][0] == 1.0f);
    TINKER_TEST_ASSERT(cf[0][1] == 2.0f);
    TINKER_TEST_ASSERT(cf[1][0] == 3.0f);
    TINKER_TEST_ASSERT(cf[1][1] == 4.0f);

    v2i ai = v2i(1, 2);
    v2i bi = v2i(3, 4);
    m2i ci = m2i(ai, bi);
    TINKER_TEST_ASSERT(ci[0] == ai);
    TINKER_TEST_ASSERT(ci[1] == bi);
    TINKER_TEST_ASSERT(ci[0][0] == 1);
    TINKER_TEST_ASSERT(ci[0][1] == 2);
    TINKER_TEST_ASSERT(ci[1][0] == 3);
    TINKER_TEST_ASSERT(ci[1][1] == 4);

    v2ui aui = v2ui(1, 2);
    v2ui bui = v2ui(3, 4);
    m2ui cui = m2ui(aui, bui);
    TINKER_TEST_ASSERT(cui[0] == aui);
    TINKER_TEST_ASSERT(cui[1] == bui);
    TINKER_TEST_ASSERT(cui[0][0] == 1);
    TINKER_TEST_ASSERT(cui[0][1] == 2);
    TINKER_TEST_ASSERT(cui[1][0] == 3);
    TINKER_TEST_ASSERT(cui[1][1] == 4);
}

void Test_m2OpAdd()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af + bf;
    TINKER_TEST_ASSERT(cf.m_data[0] == 6.0f);
    TINKER_TEST_ASSERT(cf.m_data[1] == 8.0f);
    TINKER_TEST_ASSERT(cf.m_data[2] == 10.0f);
    TINKER_TEST_ASSERT(cf.m_data[3] == 12.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai + bi;
    TINKER_TEST_ASSERT(ci.m_data[0] == 6);
    TINKER_TEST_ASSERT(ci.m_data[1] == 8);
    TINKER_TEST_ASSERT(ci.m_data[2] == 10);
    TINKER_TEST_ASSERT(ci.m_data[3] == 12);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    m2ui cui = aui + bui;
    TINKER_TEST_ASSERT(cui.m_data[0] == 6);
    TINKER_TEST_ASSERT(cui.m_data[1] == 8);
    TINKER_TEST_ASSERT(cui.m_data[2] == 10);
    TINKER_TEST_ASSERT(cui.m_data[3] == 12);
}

void Test_m2OpSub()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af - bf;
    TINKER_TEST_ASSERT(cf.m_data[0] == -4.0f);
    TINKER_TEST_ASSERT(cf.m_data[1] == -4.0f);
    TINKER_TEST_ASSERT(cf.m_data[2] == -4.0f);
    TINKER_TEST_ASSERT(cf.m_data[3] == -4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai - bi;
    TINKER_TEST_ASSERT(ci.m_data[0] == -4);
    TINKER_TEST_ASSERT(ci.m_data[1] == -4);
    TINKER_TEST_ASSERT(ci.m_data[2] == -4);
    TINKER_TEST_ASSERT(ci.m_data[3] == -4);

    m2ui aui = m2ui(5, 6, 7, 8);
    m2ui bui = m2ui(1, 2, 3, 4);
    m2ui cui = aui - bui;
    TINKER_TEST_ASSERT(cui.m_data[0] == 4);
    TINKER_TEST_ASSERT(cui.m_data[1] == 4);
    TINKER_TEST_ASSERT(cui.m_data[2] == 4);
    TINKER_TEST_ASSERT(cui.m_data[3] == 4);
}

void Test_m2OpMulM2()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af * bf;
    TINKER_TEST_ASSERT(cf.m_data[0] == 23.0f);
    TINKER_TEST_ASSERT(cf.m_data[1] == 34.0f);
    TINKER_TEST_ASSERT(cf.m_data[2] == 31.0f);
    TINKER_TEST_ASSERT(cf.m_data[3] == 46.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai * bi;
    TINKER_TEST_ASSERT(ci.m_data[0] == 23);
    TINKER_TEST_ASSERT(ci.m_data[1] == 34);
    TINKER_TEST_ASSERT(ci.m_data[2] == 31);
    TINKER_TEST_ASSERT(ci.m_data[3] == 46);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    m2ui cui = aui * bui;
    TINKER_TEST_ASSERT(cui.m_data[0] == 23);
    TINKER_TEST_ASSERT(cui.m_data[1] == 34);
    TINKER_TEST_ASSERT(cui.m_data[2] == 31);
    TINKER_TEST_ASSERT(cui.m_data[3] == 46);
}

void Test_m2OpMulV2()
{
    v2f af = v2f(1.0f, 0.0f);
    m2f bf = m2f(0.0f, 1.0f, -1.0f, 0.0f);
    v2f cf = bf * af;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, 0.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, 1.0f));
    cf = bf * cf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, -1.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, 0.0f));
    cf = bf * cf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, 0.0f));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, -1.0f));
    cf = bf * cf;
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.x, af.x));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(cf.y, af.y));

    v2i ai = v2i(1, 0);
    m2i bi = m2i(0, 1, -1, 0);
    v2i ci = bi * ai;
    TINKER_TEST_ASSERT(ci.x == 0);
    TINKER_TEST_ASSERT(ci.y == 1);
    ci = bi * ci;
    TINKER_TEST_ASSERT(ci.x == -1);
    TINKER_TEST_ASSERT(ci.y == 0);
    ci = bi * ci;
    TINKER_TEST_ASSERT(ci.x == 0);
    TINKER_TEST_ASSERT(ci.y == -1);
    ci = bi * ci;
    TINKER_TEST_ASSERT(ci.x == ai.x);
    TINKER_TEST_ASSERT(ci.y == ai.y);

    v2ui aui = v2ui(1, 0);
    m2ui bui = m2ui(1, 0, 0, 1);
    v2ui cui = bui * aui;
    TINKER_TEST_ASSERT(cui.x == 1);
    TINKER_TEST_ASSERT(cui.y == 0);
    cui = bui * cui;
    TINKER_TEST_ASSERT(cui.x == 1);
    TINKER_TEST_ASSERT(cui.y == 0);
}

void Test_m2V2MulSIMD()
{
    alignas(16) m2f mf = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    alignas(16) v2f vf = v2f(5.0f, 6.0f);
    v2f resultf1 = mf * vf;
    alignas(16) v2f resultf2;
    VectorOps::Mul_SIMD(&vf, &mf, &resultf2);
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[0], resultf2[0]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[1], resultf2[1]));

    alignas(16) m2i mi = m2i(1, 2, 3, 4);
    alignas(16) v2i vi = v2i(5, 6);
    v2i resulti1 = mi * vi;
    alignas(16) v2i resulti2;
    VectorOps::Mul_SIMD(&vi, &mi, &resulti2);
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resulti1[0], resulti2[0]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resulti1[1], resulti2[1]));

    alignas(16) m2ui mui = m2ui(1, 2, 3, 4);
    alignas(16) v2ui vui = v2ui(5, 6);
    v2ui resultui1 = mui * vui;
    alignas(16) v2ui resultui2;
    VectorOps::Mul_SIMD(&vui, &mui, &resultui2);
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultui1[0], resultui2[0]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultui1[1], resultui2[1]));
}

void Test_m2OpDiv()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af / bf;
    TINKER_TEST_ASSERT(cf.m_data[0] == 1.0f / 5.0f);
    TINKER_TEST_ASSERT(cf.m_data[1] == 1.0f / 3.0f);
    TINKER_TEST_ASSERT(cf.m_data[2] == 3.0f / 7.0f);
    TINKER_TEST_ASSERT(cf.m_data[3] == 0.5f);

    m2i ai = m2i(2, 4, 6, 8);
    m2i bi = m2i(1, 2, 3, 4);
    m2i ci = ai / bi;
    TINKER_TEST_ASSERT(ci.m_data[0] == 2);
    TINKER_TEST_ASSERT(ci.m_data[1] == 2);
    TINKER_TEST_ASSERT(ci.m_data[2] == 2);
    TINKER_TEST_ASSERT(ci.m_data[3] == 2);

    m2ui aui = m2ui(2, 4, 6, 8);
    m2ui bui = m2ui(1, 2, 3, 4);
    m2ui cui = aui / bui;
    TINKER_TEST_ASSERT(cui.m_data[0] == 2);
    TINKER_TEST_ASSERT(cui.m_data[1] == 2);
    TINKER_TEST_ASSERT(cui.m_data[2] == 2);
    TINKER_TEST_ASSERT(cui.m_data[3] == 2);
}

void Test_m2OpAddEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af += bf;
    TINKER_TEST_ASSERT(af.m_data[0] == 6.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == 8.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == 10.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == 12.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai += bi;
    TINKER_TEST_ASSERT(ai.m_data[0] == 6);
    TINKER_TEST_ASSERT(ai.m_data[1] == 8);
    TINKER_TEST_ASSERT(ai.m_data[2] == 10);
    TINKER_TEST_ASSERT(ai.m_data[3] == 12);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui += bui;
    TINKER_TEST_ASSERT(aui.m_data[0] == 6);
    TINKER_TEST_ASSERT(aui.m_data[1] == 8);
    TINKER_TEST_ASSERT(aui.m_data[2] == 10);
    TINKER_TEST_ASSERT(aui.m_data[3] == 12);
}

void Test_m2OpSubEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af -= bf;
    TINKER_TEST_ASSERT(af.m_data[0] == -4.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == -4.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == -4.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == -4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai -= bi;
    TINKER_TEST_ASSERT(ai.m_data[0] == -4);
    TINKER_TEST_ASSERT(ai.m_data[1] == -4);
    TINKER_TEST_ASSERT(ai.m_data[2] == -4);
    TINKER_TEST_ASSERT(ai.m_data[3] == -4);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui -= bui;
    TINKER_TEST_ASSERT(aui.m_data[0] == -4);
    TINKER_TEST_ASSERT(aui.m_data[1] == -4);
    TINKER_TEST_ASSERT(aui.m_data[2] == -4);
    TINKER_TEST_ASSERT(aui.m_data[3] == -4);
}

void Test_m2OpMulEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af *= bf;
    TINKER_TEST_ASSERT(af.m_data[0] == 5.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == 12.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == 21.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == 32.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai *= bi;
    TINKER_TEST_ASSERT(ai.m_data[0] == 5);
    TINKER_TEST_ASSERT(ai.m_data[1] == 12);
    TINKER_TEST_ASSERT(ai.m_data[2] == 21);
    TINKER_TEST_ASSERT(ai.m_data[3] == 32);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui *= bui;
    TINKER_TEST_ASSERT(aui.m_data[0] == 5);
    TINKER_TEST_ASSERT(aui.m_data[1] == 12);
    TINKER_TEST_ASSERT(aui.m_data[2] == 21);
    TINKER_TEST_ASSERT(aui.m_data[3] == 32);
}

void Test_m2OpDivEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af /= bf;
    TINKER_TEST_ASSERT(af.m_data[0] == 1.0f / 5.0f);
    TINKER_TEST_ASSERT(af.m_data[1] == 1.0f / 3.0f);
    TINKER_TEST_ASSERT(af.m_data[2] == 3.0f / 7.0f);
    TINKER_TEST_ASSERT(af.m_data[3] == 0.5f);

    m2i ai = m2i(2, 4, 6, 8);
    m2i bi = m2i(1, 2, 3, 4);
    ai /= bi;
    TINKER_TEST_ASSERT(ai.m_data[0] == 2);
    TINKER_TEST_ASSERT(ai.m_data[1] == 2);
    TINKER_TEST_ASSERT(ai.m_data[2] == 2);
    TINKER_TEST_ASSERT(ai.m_data[3] == 2);

    m2ui aui = m2ui(2, 4, 6, 8);
    m2ui bui = m2ui(1, 2, 3, 4);
    aui /= bui;
    TINKER_TEST_ASSERT(aui.m_data[0] == 2);
    TINKER_TEST_ASSERT(aui.m_data[1] == 2);
    TINKER_TEST_ASSERT(aui.m_data[2] == 2);
    TINKER_TEST_ASSERT(aui.m_data[3] == 2);
}

void Test_m2Eq()
{
    m2f af = m2f(1.0f, -2.0f, 3.0f, -4.0f);
    m2f bf = m2f(1.0f, -2.0f, 3.0f, -4.0f);
    TINKER_TEST_ASSERT(af == bf);

    m2i ai = m2i(1, -2, 3, -4);
    m2i bi = m2i(1, -2, 3, -4);
    TINKER_TEST_ASSERT(ai == bi);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(1, 2, 3, 4);
    TINKER_TEST_ASSERT(aui == bui);
}

void Test_m2NEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(-1.0f, -2.0f, -3.0f, -4.0f);
    TINKER_TEST_ASSERT(af != bf);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(-1, -2, -3, -4);
    TINKER_TEST_ASSERT(ai != bi);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    TINKER_TEST_ASSERT(aui != bui);
}

void Test_m4V4MulSIMD()
{
    alignas(16) m4f mf = m4f(1.0f, 2.0f, 3.0f, 4.0f,
		 5.0f, 6.0f, 7.0f, 8.0f,
		 9.0f, 10.0f, 11.0f, 12.0f,
		 13.0f, 14.0f, 15.0f, 16.0f);
    alignas(16) v4f vf = v4f(5.0f, 6.0f, 7.0f, 8.0f);
    v4f resultf1 = mf * vf;
    alignas(16) v4f resultf2;
    VectorOps::Mul_SIMD(&vf, &mf, &resultf2);
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[0], resultf2[0]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[1], resultf2[1]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[2], resultf2[2]));
    TINKER_TEST_ASSERT(FLOAT_EQUAL(resultf1[3], resultf2[3]));


    alignas(16) m4i mi = m4i(1, 2, 3, 4,
		 5, 6, 7, 8,
		 9, 10, 11, 12,
		 13, 14, 15, 16);
    alignas(16) v4i vi = v4i(5, 6, 7, 8);
    v4i resulti1 = mi * vi;
    alignas(16) v4i resulti2;
    VectorOps::Mul_SIMD(&vi, &mi, &resulti2);
    TINKER_TEST_ASSERT(resulti1[0] == resulti2[0]);
    TINKER_TEST_ASSERT(resulti1[1] == resulti2[1]);
    TINKER_TEST_ASSERT(resulti1[2] == resulti2[2]);
    TINKER_TEST_ASSERT(resulti1[3] == resulti2[3]);

    alignas(16) m4ui mui = m4ui(1, 2, 3, 4,
		 5, 6, 7, 8,
		 9, 10, 11, 12,
		 13, 14, 15, 16);
    alignas(16) v4ui vui = v4ui(5, 6, 7, 8);
    v4ui resultui1 = mui * vui;
    alignas(16) v4ui resultui2;
    VectorOps::Mul_SIMD(&vui, &mui, &resultui2);
    TINKER_TEST_ASSERT(resultui1[0] == resultui2[0]);
    TINKER_TEST_ASSERT(resultui1[1] == resultui2[1]);
    TINKER_TEST_ASSERT(resultui1[2] == resultui2[2]);
    TINKER_TEST_ASSERT(resultui1[3] == resultui2[3]);
}

