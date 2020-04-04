#pragma once

#include "../System/CoreTypes.h"

// 2D Vector
template <typename T>
class v2
{
public:
    union
    {
        T m_data[2];
        struct
        {
            T x;
            T y;
        };
    };

    v2()
    {
        m_data = {};
    }

    // TODO: copy and move constructors
    v2(T x, T y)
    {
        this->x = x;
        this->y = y;
    }

    // TODO: operator overloads
    v2<T> operator+(const v2<T>& other)
    {
        return { x + other.x, y + other.y };
    }

    void operator+=(const v2<T>& other)
    {
        x += other.x;
        y += other.y;
    }
};

typedef v2<uint32> v2ui;
typedef v2<int32> v2i;
typedef v2<float> v2f;
