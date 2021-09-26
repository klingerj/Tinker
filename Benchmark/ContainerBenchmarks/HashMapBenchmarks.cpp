#include "HashMapBenchmarks.h"
#include "Core/DataStructures/HashMap.h"

#include <unordered_map>

using namespace Tk;
using namespace Core;

const uint32 g_numEles = 10000000;
uint32 keys[g_numEles] = {};
uint32 vals[g_numEles] = {};

void BM_hm_Startup()
{
    for (uint32 i = 0; i < g_numEles; ++i)
    {
        keys[i] = rand();
        vals[i] = rand();
    }
}

void BM_hm_Shutdown()
{

}

uint32 BM_HMIns1M_fScalar()
{
    HashMap<uint32, uint32> map;
    map.Reserve(g_numEles);

    for (uint32 i = 0; i < g_numEles; ++i)
    {
        map.Insert(keys[i], vals[i]);
    }

    return map.FindIndex(keys[256]);
}

void BM_StdHMIns1M_fScalar()
{
    std::unordered_map<uint32, uint32> map_std;
    map_std.reserve(g_numEles);

    for (uint32 i = 0; i < g_numEles; ++i)
    {
        map_std[keys[i]] = vals[i];
    }
}
