#include "Core/Containers/HashMap.h"
#include "TinkerTest.h"

using namespace Containers;

#include <unordered_map>
#include <random>
void Test_HashMap_Basic()
{
    const uint32 numEles = 65536;
    uint32 keys[numEles] = {};
    uint32 vals[numEles] = {};
    for (uint32 i = 0; i < numEles; ++i)
    {
        keys[i] = rand() % UINT32_MAX;
        vals[i] = rand() % UINT32_MAX;
    }
    
    HashMap<uint32, uint32> map;
    map.Reserve(numEles);

    std::unordered_map<uint32, uint32> map_std;
    map_std.reserve(numEles);

    for (uint32 i = 0; i < numEles; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles; ++i)
    {
        uint32 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint32 value = map.DataAtIndex(key);

        uint32 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }

    // then change all the vals and overwrite all keys
    for (uint32 i = 0; i < numEles; ++i)
    {
        vals[i] = rand() % UINT32_MAX;
    }

    for (uint32 i = 0; i < numEles; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles; ++i)
    {
        uint32 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint32 value = map.DataAtIndex(key);

        uint32 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }
}
