#include "../../Source/Math/VectorTypes.h"
#include "../TinkerTest.h"

// V2
void Test_v2ConstructorDefault()
{
    v2f af;
    TINKER_ASSERT(af.x == 0.0f);
    TINKER_ASSERT(af.y == 0.0f);

    v2i ai;
    TINKER_ASSERT(ai.x == 0);
    TINKER_ASSERT(ai.y == 0);

    v2ui aui;
    TINKER_ASSERT(aui.x == 0);
    TINKER_ASSERT(aui.y == 0);
}

void Test_v2ConstructorV2()
{
    v2f af;
    af.x = 1.0f;
    af.y = 2.0f;
    v2f bf = v2f(af);
    TINKER_ASSERT(af.x == bf.x);
    TINKER_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 2;
    v2i bi = v2i(ai);
    TINKER_ASSERT(ai.x == bi.x);
    TINKER_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 2;
    v2ui bui = v2ui(aui);
    TINKER_ASSERT(aui.x == bui.x);
    TINKER_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorArray()
{
    v2f af;
    af.x = 1.0f;
    af.y = 1.0f;
    v2f bf = v2f(af.m_data);
    TINKER_ASSERT(af.x == bf.x);
    TINKER_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 1;
    v2i bi = v2i(ai.m_data);
    TINKER_ASSERT(ai.x == bi.x);
    TINKER_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 1;
    v2ui bui = v2ui(aui.m_data);
    TINKER_ASSERT(aui.x == bui.x);
    TINKER_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorF1()
{
    v2f af;
    af.x = 1.0f;
    af.y = 1.0f;
    v2f bf = v2f(af.x);
    TINKER_ASSERT(af.x == bf.x);
    TINKER_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 1;
    v2i bi = v2i(ai.x);
    TINKER_ASSERT(ai.x == bi.x);
    TINKER_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 1;
    v2ui bui = v2ui(aui.x);
    TINKER_ASSERT(aui.x == bui.x);
    TINKER_ASSERT(aui.y == bui.y);
}

void Test_v2ConstructorF2()
{
    v2f af;
    af.x = 1.0f;
    af.y = 1.0f;
    v2f bf = v2f(af.x, af.y);
    TINKER_ASSERT(af.x == bf.x);
    TINKER_ASSERT(af.y == bf.y);

    v2i ai;
    ai.x = 1;
    ai.y = 1;
    v2i bi = v2i(ai.x, ai.y);
    TINKER_ASSERT(ai.x == bi.x);
    TINKER_ASSERT(ai.y == bi.y);

    v2ui aui;
    aui.x = 1;
    aui.y = 1;
    v2ui bui = v2ui(aui.x, aui.y);
    TINKER_ASSERT(aui.x == bui.x);
    TINKER_ASSERT(aui.y == bui.y);
}

void Test_v2OpAdd()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af + bf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, 2.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, -2.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai + bi;
    TINKER_ASSERT(ci.x == 2);
    TINKER_ASSERT(ci.y == -2);

    v2ui aui(1, 2);
    v2ui bui(3, 4);
    v2ui cui = aui + bui;
    TINKER_ASSERT(cui.x == 4);
    TINKER_ASSERT(cui.y == 6);
}

void Test_v2OpSub()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af - bf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, -4.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, 6.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai - bi;
    TINKER_ASSERT(ci.x == -4);
    TINKER_ASSERT(ci.y == 6);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui - bui;
    TINKER_ASSERT(cui.x == 2);
    TINKER_ASSERT(cui.y == 2);
}

void Test_v2OpMul()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af * bf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, -3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, -8.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    v2i ci = ai * bi;
    TINKER_ASSERT(ci.x == -3);
    TINKER_ASSERT(ci.y == -8);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui * bui;
    TINKER_ASSERT(cui.x == 3);
    TINKER_ASSERT(cui.y == 8);
}

void Test_v2OpDiv()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    v2f cf = af / bf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, -1.0f / 3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, -0.5f));

    v2i ai(-3, 4);
    v2i bi(1, -2);
    v2i ci = ai / bi;
    TINKER_ASSERT(ci.x == -3);
    TINKER_ASSERT(ci.y == -2);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    v2ui cui = aui / bui;
    TINKER_ASSERT(cui.x == 3);
    TINKER_ASSERT(cui.y == 2);
}

void Test_v2OpAddEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af += bf;
    TINKER_ASSERT(FLOAT_EQUAL(af.x, 2.0f));
    TINKER_ASSERT(FLOAT_EQUAL(af.y, -2.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai += bi;
    TINKER_ASSERT(ai.x == 2);
    TINKER_ASSERT(ai.y == -2);

    v2ui aui(1, 2);
    v2ui bui(3, 4);
    aui += bui;
    TINKER_ASSERT(aui.x == 4);
    TINKER_ASSERT(aui.y == 6);
}

void Test_v2OpSubEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af -= bf;
    TINKER_ASSERT(FLOAT_EQUAL(af.x, -4.0f));
    TINKER_ASSERT(FLOAT_EQUAL(af.y, 6.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai -= bi;
    TINKER_ASSERT(ai.x == -4);
    TINKER_ASSERT(ai.y == 6);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui -= bui;
    TINKER_ASSERT(aui.x == 2);
    TINKER_ASSERT(aui.y == 2);
}

void Test_v2OpMulEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af *= bf;
    TINKER_ASSERT(FLOAT_EQUAL(af.x, -3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(af.y, -8.0f));

    v2i ai(-1, 2);
    v2i bi(3, -4);
    ai *= bi;
    TINKER_ASSERT(ai.x == -3);
    TINKER_ASSERT(ai.y == -8);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui *= bui;
    TINKER_ASSERT(aui.x == 3);
    TINKER_ASSERT(aui.y == 8);
}

void Test_v2OpDivEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(3.0f, -4.0f);
    af /= bf;
    TINKER_ASSERT(FLOAT_EQUAL(af.x, -1.0f / 3.0f));
    TINKER_ASSERT(FLOAT_EQUAL(af.y, -0.5f));

    v2i ai(-3, 4);
    v2i bi(1, -2);
    ai /= bi;
    TINKER_ASSERT(ai.x == -3);
    TINKER_ASSERT(ai.y == -2);

    v2ui aui(3, 4);
    v2ui bui(1, 2);
    aui /= bui;
    TINKER_ASSERT(aui.x == 3);
    TINKER_ASSERT(aui.y == 2);
}

void Test_v2Eq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(-1.0f, 2.0f);
    TINKER_ASSERT(af == bf);

    v2f ai(-1, 2);
    v2f bi(-1, 2);
    TINKER_ASSERT(ai == bi);

    v2f aui(1, 2);
    v2f bui(1, 2);
    TINKER_ASSERT(aui == bui);
}

void Test_v2NEq()
{
    v2f af(-1.0f, 2.0f);
    v2f bf(0.0f, 0.0f);
    TINKER_ASSERT(af != bf);

    v2f ai(-1, 2);
    v2f bi(0, 0);
    TINKER_ASSERT(ai != bi);

    v2f aui(1, 2);
    v2f bui(0, 0);
    TINKER_ASSERT(aui != bui);
}

// M2
void Test_m2ConstructorDefault()
{
    m2f af;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(af.m_data[i] == 0.0f);
    }

    m2i ai;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(ai.m_data[i] == 0);
    }

    m2ui aui;
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(aui.m_data[i] == 0);
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
        TINKER_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[1] = 2;
    ai.m_data[2] = 3;
    ai.m_data[3] = 4;

    m2i bi = m2i(ai);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[1] = 2;
    aui.m_data[2] = 3;
    aui.m_data[3] = 4;

    m2ui bui = m2ui(aui);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bui.m_data[i] == aui.m_data[i]);
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
        TINKER_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[1] = 2;
    ai.m_data[2] = 3;
    ai.m_data[3] = 4;

    m2i bi = m2i(ai.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[1] = 2;
    aui.m_data[2] = 3;
    aui.m_data[3] = 4;

    m2ui bui = m2ui(aui.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bui.m_data[i] == aui.m_data[i]);
    }
}

void Test_m2ConstructorF1()
{
    m2f af;
    af.m_data[0] = 1.0f;
    af.m_data[1] = 2.0f;
    af.m_data[2] = 3.0f;
    af.m_data[3] = 4.0f;

    m2f bf = m2f(af.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bf.m_data[i] == af.m_data[i]);
    }

    m2i ai;
    ai.m_data[0] = 1;
    ai.m_data[1] = 2;
    ai.m_data[2] = 3;
    ai.m_data[3] = 4;

    m2i bi = m2i(ai.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bi.m_data[i] == ai.m_data[i]);
    }

    m2ui aui;
    aui.m_data[0] = 1;
    aui.m_data[1] = 2;
    aui.m_data[2] = 3;
    aui.m_data[3] = 4;

    m2ui bui = m2ui(aui.m_data);
    for (uint8 i = 0; i < 4; ++i)
    {
        TINKER_ASSERT(bui.m_data[i] == aui.m_data[i]);
    }
}

void Test_m2Constructor2V2()
{
    m2f af = m2f(v2f(1.0f, 2.0f), v2f(3.0f, 4.0f));
    TINKER_ASSERT(af.m_data[0] == 1.0f);
    TINKER_ASSERT(af.m_data[1] == 2.0f);
    TINKER_ASSERT(af.m_data[2] == 3.0f);
    TINKER_ASSERT(af.m_data[3] == 4.0f);

    m2i ai = m2i(v2i(1, 2), v2i(3, 4));
    TINKER_ASSERT(ai.m_data[0] == 1);
    TINKER_ASSERT(ai.m_data[1] == 2);
    TINKER_ASSERT(ai.m_data[2] == 3);
    TINKER_ASSERT(ai.m_data[3] == 4);

    m2ui aui = m2ui(v2ui(1, 2), v2ui(3, 4));
    TINKER_ASSERT(aui.m_data[0] == 1);
    TINKER_ASSERT(aui.m_data[1] == 2);
    TINKER_ASSERT(aui.m_data[2] == 3);
    TINKER_ASSERT(aui.m_data[3] == 4);
}

void Test_m2ConstructorF4()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    TINKER_ASSERT(af.m_data[0] == 1.0f);
    TINKER_ASSERT(af.m_data[1] == 2.0f);
    TINKER_ASSERT(af.m_data[2] == 3.0f);
    TINKER_ASSERT(af.m_data[3] == 4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    TINKER_ASSERT(ai.m_data[0] == 1);
    TINKER_ASSERT(ai.m_data[1] == 2);
    TINKER_ASSERT(ai.m_data[2] == 3);
    TINKER_ASSERT(ai.m_data[3] == 4);

    m2ui aui = m2ui(1, 2, 3, 4);
    TINKER_ASSERT(aui.m_data[0] == 1);
    TINKER_ASSERT(aui.m_data[1] == 2);
    TINKER_ASSERT(aui.m_data[2] == 3);
    TINKER_ASSERT(aui.m_data[3] == 4);
}

void Test_m2OpAdd()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af + bf;
    TINKER_ASSERT(cf.m_data[0] == 6.0f);
    TINKER_ASSERT(cf.m_data[1] == 8.0f);
    TINKER_ASSERT(cf.m_data[2] == 10.0f);
    TINKER_ASSERT(cf.m_data[3] == 12.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai + bi;
    TINKER_ASSERT(ci.m_data[0] == 6);
    TINKER_ASSERT(ci.m_data[1] == 8);
    TINKER_ASSERT(ci.m_data[2] == 10);
    TINKER_ASSERT(ci.m_data[3] == 12);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    m2ui cui = aui + bui;
    TINKER_ASSERT(cui.m_data[0] == 6);
    TINKER_ASSERT(cui.m_data[1] == 8);
    TINKER_ASSERT(cui.m_data[2] == 10);
    TINKER_ASSERT(cui.m_data[3] == 12);
}

void Test_m2OpSub()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af - bf;
    TINKER_ASSERT(cf.m_data[0] == -4.0f);
    TINKER_ASSERT(cf.m_data[1] == -4.0f);
    TINKER_ASSERT(cf.m_data[2] == -4.0f);
    TINKER_ASSERT(cf.m_data[3] == -4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai - bi;
    TINKER_ASSERT(ci.m_data[0] == -4);
    TINKER_ASSERT(ci.m_data[1] == -4);
    TINKER_ASSERT(ci.m_data[2] == -4);
    TINKER_ASSERT(ci.m_data[3] == -4);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    m2ui cui = aui - bui;
    TINKER_ASSERT(cui.m_data[0] == -4);
    TINKER_ASSERT(cui.m_data[1] == -4);
    TINKER_ASSERT(cui.m_data[2] == -4);
    TINKER_ASSERT(cui.m_data[3] == -4);
}

void Test_m2OpMulM2()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af * bf;
    TINKER_ASSERT(cf.m_data[0] == 5.0f);
    TINKER_ASSERT(cf.m_data[1] == 12.0f);
    TINKER_ASSERT(cf.m_data[2] == 21.0f);
    TINKER_ASSERT(cf.m_data[3] == 32.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    m2i ci = ai * bi;
    TINKER_ASSERT(ci.m_data[0] == 5);
    TINKER_ASSERT(ci.m_data[1] == 12);
    TINKER_ASSERT(ci.m_data[2] == 21);
    TINKER_ASSERT(ci.m_data[3] == 32);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    m2ui cui = aui * bui;
    TINKER_ASSERT(cui.m_data[0] == 5);
    TINKER_ASSERT(cui.m_data[1] == 12);
    TINKER_ASSERT(cui.m_data[2] == 21);
    TINKER_ASSERT(cui.m_data[3] == 32);
}

void Test_m2OpMulV2()
{
    v2f af = v2f(1.0f, 0.0f);
    m2f bf = m2f(0.0f, -1.0f, 1.0f, 0.0f);
    v2f cf = bf * af;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, 0.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, 1.0f));
    cf = bf * cf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, -1.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, 0.0f));
    cf = bf * cf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, 0.0f));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, -1.0f));
    cf = bf * cf;
    TINKER_ASSERT(FLOAT_EQUAL(cf.x, af.x));
    TINKER_ASSERT(FLOAT_EQUAL(cf.y, af.y));

    v2i ai = v2i(1, 0);
    m2i bi = m2i(0, -1, 1, 0);
    v2i ci = bi * ai;
    TINKER_ASSERT(ci.x == 0);
    TINKER_ASSERT(ci.y == 1);
    ci = bi * ci;
    TINKER_ASSERT(ci.x == -1);
    TINKER_ASSERT(ci.y == 0);
    ci = bi * ci;
    TINKER_ASSERT(ci.x == 0);
    TINKER_ASSERT(ci.y == -1);
    ci = bi * ci;
    TINKER_ASSERT(ci.x == ai.x);
    TINKER_ASSERT(ci.y == ai.y);

    v2ui aui = v2ui(1, 0);
    m2ui bui = m2ui(1, 0, 0, 1);
    v2ui cui = bui * aui;
    TINKER_ASSERT(cui.x == 1);
    TINKER_ASSERT(cui.y == 0);
    cui = bui * cui;
    TINKER_ASSERT(cui.x == 1);
    TINKER_ASSERT(cui.y == 0);
}

void Test_m2OpDiv()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    m2f cf = af / bf;
    TINKER_ASSERT(cf.m_data[0] == 1.0f / 5.0f);
    TINKER_ASSERT(cf.m_data[1] == 1.0f / 3.0f);
    TINKER_ASSERT(cf.m_data[2] == 3.0f / 7.0f);
    TINKER_ASSERT(cf.m_data[3] == 0.5f);

    m2i ai = m2i(2, 4, 6, 8);
    m2i bi = m2i(1, 2, 3, 4);
    m2i ci = ai / bi;
    TINKER_ASSERT(ci.m_data[0] == 2);
    TINKER_ASSERT(ci.m_data[1] == 2);
    TINKER_ASSERT(ci.m_data[2] == 2);
    TINKER_ASSERT(ci.m_data[3] == 2);

    m2ui aui = m2ui(2, 4, 6, 8);
    m2ui bui = m2ui(1, 2, 3, 4);
    m2ui cui = aui / bui;
    TINKER_ASSERT(cui.m_data[0] == 2);
    TINKER_ASSERT(cui.m_data[1] == 2);
    TINKER_ASSERT(cui.m_data[2] == 2);
    TINKER_ASSERT(cui.m_data[3] == 2);
}

void Test_m2OpAddEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af += bf;
    TINKER_ASSERT(af.m_data[0] == 6.0f);
    TINKER_ASSERT(af.m_data[1] == 8.0f);
    TINKER_ASSERT(af.m_data[2] == 10.0f);
    TINKER_ASSERT(af.m_data[3] == 12.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai += bi;
    TINKER_ASSERT(ai.m_data[0] == 6);
    TINKER_ASSERT(ai.m_data[1] == 8);
    TINKER_ASSERT(ai.m_data[2] == 10);
    TINKER_ASSERT(ai.m_data[3] == 12);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui += bui;
    TINKER_ASSERT(aui.m_data[0] == 6);
    TINKER_ASSERT(aui.m_data[1] == 8);
    TINKER_ASSERT(aui.m_data[2] == 10);
    TINKER_ASSERT(aui.m_data[3] == 12);
}

void Test_m2OpSubEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af -= bf;
    TINKER_ASSERT(af.m_data[0] == -4.0f);
    TINKER_ASSERT(af.m_data[1] == -4.0f);
    TINKER_ASSERT(af.m_data[2] == -4.0f);
    TINKER_ASSERT(af.m_data[3] == -4.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai -= bi;
    TINKER_ASSERT(ai.m_data[0] == -4);
    TINKER_ASSERT(ai.m_data[1] == -4);
    TINKER_ASSERT(ai.m_data[2] == -4);
    TINKER_ASSERT(ai.m_data[3] == -4);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui -= bui;
    TINKER_ASSERT(aui.m_data[0] == -4);
    TINKER_ASSERT(aui.m_data[1] == -4);
    TINKER_ASSERT(aui.m_data[2] == -4);
    TINKER_ASSERT(aui.m_data[3] == -4);
}

void Test_m2OpMulEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af *= bf;
    TINKER_ASSERT(af.m_data[0] == 5.0f);
    TINKER_ASSERT(af.m_data[1] == 12.0f);
    TINKER_ASSERT(af.m_data[2] == 21.0f);
    TINKER_ASSERT(af.m_data[3] == 32.0f);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(5, 6, 7, 8);
    ai *= bi;
    TINKER_ASSERT(ai.m_data[0] == 5);
    TINKER_ASSERT(ai.m_data[1] == 12);
    TINKER_ASSERT(ai.m_data[2] == 21);
    TINKER_ASSERT(ai.m_data[3] == 32);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    aui *= bui;
    TINKER_ASSERT(aui.m_data[0] == 5);
    TINKER_ASSERT(aui.m_data[1] == 12);
    TINKER_ASSERT(aui.m_data[2] == 21);
    TINKER_ASSERT(aui.m_data[3] == 32);
}

void Test_m2OpDivEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(5.0f, 6.0f, 7.0f, 8.0f);
    af /= bf;
    TINKER_ASSERT(af.m_data[0] == 1.0f / 5.0f);
    TINKER_ASSERT(af.m_data[1] == 1.0f / 3.0f);
    TINKER_ASSERT(af.m_data[2] == 3.0f / 7.0f);
    TINKER_ASSERT(af.m_data[3] == 0.5f);

    m2i ai = m2i(2, 4, 6, 8);
    m2i bi = m2i(1, 2, 3, 4);
    ai /= bi;
    TINKER_ASSERT(ai.m_data[0] == 2);
    TINKER_ASSERT(ai.m_data[1] == 2);
    TINKER_ASSERT(ai.m_data[2] == 2);
    TINKER_ASSERT(ai.m_data[3] == 2);

    m2ui aui = m2ui(2, 4, 6, 8);
    m2ui bui = m2ui(1, 2, 3, 4);
    aui /= bui;
    TINKER_ASSERT(aui.m_data[0] == 2);
    TINKER_ASSERT(aui.m_data[1] == 2);
    TINKER_ASSERT(aui.m_data[2] == 2);
    TINKER_ASSERT(aui.m_data[3] == 2);
}

void Test_m2Eq()
{
    m2f af = m2f(1.0f, -2.0f, 3.0f, -4.0f);
    m2f bf = m2f(1.0f, -2.0f, 3.0f, -4.0f);
    TINKER_ASSERT(af == bf);

    m2i ai = m2i(1, -2, 3, -4);
    m2i bi = m2i(1, -2, 3, -4);
    TINKER_ASSERT(ai == bi);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(1, 2, 3, 4);
    TINKER_ASSERT(aui == bui);
}

void Test_m2NEq()
{
    m2f af = m2f(1.0f, 2.0f, 3.0f, 4.0f);
    m2f bf = m2f(-1.0f, -2.0f, -3.0f, -4.0f);
    TINKER_ASSERT(af != bf);

    m2i ai = m2i(1, 2, 3, 4);
    m2i bi = m2i(-1, -2, -3, -4);
    TINKER_ASSERT(ai != bi);

    m2ui aui = m2ui(1, 2, 3, 4);
    m2ui bui = m2ui(5, 6, 7, 8);
    TINKER_ASSERT(aui != bui);
}
