#include "DataStructures/HashMap.h"
#include "TinkerTest.h"

#include <unordered_map>
#include <random>
void Test_HashMap_Basic_U32()
{
    const uint32 numEles = 65536;
    uint32* keys = new uint32[numEles];
    uint32* vals = new uint32[numEles];
    for (uint32 i = 0; i < numEles; ++i)
    {
        keys[i] = i;
        vals[i] = i;
    }

    HashMapU32 map;
    map.Reserve(numEles);

    std::unordered_map<uint32, uint32> map_std;
    map_std.reserve(numEles);

    // Insert half of the elements
    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        uint32 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint32 value = map.DataAtIndex(index);

        uint32 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }
    // Then induce a hash collision on all the keys

    // then change all the vals and overwrite all keys
    for (uint32 i = 0; i < numEles; ++i)
    {
        keys[i] += numEles;
        vals[i] = rand();
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        uint32 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint32 value = map.DataAtIndex(index);

        uint32 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }

    delete keys;
    delete vals;
}

void Test_HashMap_Basic_U64()
{
    const uint32 numEles = 65536;
    uint64* keys = new uint64[numEles];
    uint64* vals = new uint64[numEles];
    for (uint32 i = 0; i < numEles; ++i)
    {
        keys[i] = i;
        vals[i] = i;
    }

    HashMapU64 map;
    map.Reserve(numEles);

    std::unordered_map<uint64, uint64> map_std;
    map_std.reserve(numEles);

    // Insert half of the elements
    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        uint64 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint64 value = map.DataAtIndex(index);

        uint64 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }
    // Then induce a hash collision on all the keys

    // then change all the vals and overwrite all keys
    for (uint32 i = 0; i < numEles; ++i)
    {
        keys[i] += numEles;
        vals[i] = rand();
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        map.Insert(keys[i], vals[i]);
        map_std[keys[i]] = vals[i];
    }

    for (uint32 i = 0; i < numEles / 2; ++i)
    {
        uint64 key = keys[i];

        uint32 index = map.FindIndex(key);
        TINKER_ASSERT(index != HashMapBase::eInvalidIndex);
        uint64 value = map.DataAtIndex(index);

        uint64 value_std = map_std.find(key)->second;
        TINKER_ASSERT(value == value_std);
    }

    delete keys;
    delete vals;
}

void Test_HashMap_InvalidKey()
{
    HashMapU32 map;
    map.Reserve(64);

    uint32 index = map.FindIndex(HashMapBase::eInvalidIndex);
    TINKER_ASSERT(index == HashMapBase::eInvalidIndex);
}
