#include "HashMapBenchmarks.h"
#include "DataStructures/HashMap.h"

#include <unordered_map>

using namespace Tk;
using namespace Core;

const uint32 g_numEles = 10000;
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
    map.Reserve(g_numEles * 2);

    HashMap<uint32, uint32> map2;
    map2.Reserve(g_numEles * 2);

    // insert a ton of elements into one map
    for (uint32 i = 0; i < g_numEles; ++i)
    {
        map.Insert(keys[i], vals[i]);
    }

    // read from that map and insert them into the other map, at random indices
    for (uint32 i = 0; i < g_numEles; ++i)
    {
        uint32 randIndex = rand() % g_numEles;
        uint32 key = keys[randIndex];

        uint32 dataIndex = map.FindIndex(key);
        uint32 value = map.DataAtIndex(dataIndex);
        map2.Insert(key, value);
    }

    // return some random value out of the second map
    return map2.FindIndex(keys[256]);
}

uint32 BM_StdHMIns1M_fScalar()
{
    std::unordered_map<uint32, uint32> map_std;
    map_std.reserve(g_numEles * 2);

    std::unordered_map<uint32, uint32> map_std2;
    map_std2.reserve(g_numEles * 2);

    // insert a ton of elements into one map
    for (uint32 i = 0; i < g_numEles; ++i)
    {
        map_std[keys[i]] = vals[i];
    }

    // read from that map and insert them into the other map, at random indices
    for (uint32 i = 0; i < g_numEles; ++i)
    {
        uint32 randIndex = rand() % g_numEles;
        uint32 key = keys[randIndex];

        uint32 value = map_std.at(key);
        map_std2[key] = value;
    }

    // return some random value out of the second map
    return map_std2[keys[256]];
}
