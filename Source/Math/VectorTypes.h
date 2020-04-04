#pragma once

#include "../System/CoreDefines.h"

template <typename T>
class vec2
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

    vec2()
    {
        x = {};
        y = {};
    }

    vec2(const vec2& v)
    {
        this->x = v.x;
        this->y = v.y;
    }

    vec2(const T* data)
    {
        for (uint8 i = 0; i < 2; ++i)
        {
            m_data[i] = data[i];
        }
    }

    vec2(const T& x)
    {
        this->x = x;
        this->y = x;
    }

    vec2(const T& x, const T& y)
    {
        this->x = x;
        this->y = y;
    }

    vec2<T> operator+(const vec2<T>& other)
    {
        return { x + other.x, y + other.y };
    }

    vec2<T> operator-(const vec2<T>& other)
    {
        return { x - other.x, y - other.y };
    }

    vec2<T> operator*(const vec2<T>& other)
    {
        return { x * other.x, y * other.y };
    }

    vec2<T> operator/(const vec2<T>& other)
    {
        return { x / other.x, y / other.y };
    }

    void operator+=(const vec2<T>& other)
    {
        x += other.x;
        y += other.y;
    }

    void operator-=(const vec2<T>& other)
    {
        x -= other.x;
        y -= other.y;
    }

    void operator*=(const vec2<T>& other)
    {
        x *= other.x;
        y *= other.y;
    }

    void operator/=(const vec2<T>& other)
    {
        x /= other.x;
        y /= other.y;
    }
};

template <typename T>
bool operator==(const vec2<T>& lhs, const vec2<T>& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(const vec2<T>& lhs, const vec2<T>& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
class mat2
{
public:
    T m_data[4];

    mat2()
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = {};
    }

    mat2(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = other.m_data[i];
    }

    mat2(T* data)
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = data[i];
    }

    mat2(const T& x)
    {
        m_data[0] = x;
        m_data[1] = {};
        m_data[2] = {};
        m_data[3] = x;
    }

    mat2(const vec2<T>& a, vec2<T> b)
    {
        m_data[0] = a.x;
        m_data[1] = a.y;
        m_data[2] = b.x;
        m_data[3] = b.y;
    }

    mat2(const T& x00, const T& x01, const T& x10, const T& x11)
    {
        m_data[0] = x00;
        m_data[1] = x01;
        m_data[2] = x10;
        m_data[3] = x11;
    }

    mat2<T> operator+(const mat2<T>& other)
    {
        T newData[4];
        for (uint8 i = 0; i < 4; ++i)
        {
            newData[i] = m_data[i] + other.m_data[i];
        }
        return { newData };
    }

    mat2<T> operator-(const mat2<T>& other)
    {
        T newData[4];
        for (uint8 i = 0; i < 4; ++i)
        {
            newData[i] = m_data[i] - other.m_data[i];
        }
        return { newData };
    }

    mat2<T> operator*(const mat2<T>& other)
    {
        T newData[4];
        for (uint8 i = 0; i < 4; ++i)
        {
            newData[i] = m_data[i] * other.m_data[i];
        }
        return { newData };
    }

    mat2<T> operator/(const mat2<T>& other)
    {
        T newData[4];
        for (uint8 i = 0; i < 4; ++i)
        {
            newData[i] = m_data[i] / other.m_data[i];
        }
        return { newData };
    }

    void operator+=(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            m_data[i] += other.m_data[i];
        }
    }

    void operator-=(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            m_data[i] -= other.m_data[i];
        }
    }

    void operator*=(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            m_data[i] *= other.m_data[i];
        }
    }

    void operator/=(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            m_data[i] /= other.m_data[i];
        }
    }
};

template <typename T>
vec2<T> operator*(const mat2<T>& m, const vec2<T> v)
{
    // TODO: Implement vectorized versions of this.
    return { m.m_data[0] * v.x + m.m_data[1] * v.y, m.m_data[2] * v.x + m.m_data[3] * v.y };
}

template <typename T>
bool operator==(const mat2<T>& lhs, const mat2<T>& rhs)
{
    bool equal = true;
    for (uint8 i = 0; i < 4; ++i)
    {
        equal &= lhs.m_data[i] == rhs.m_data[i];
    }
    return equal;
}

template <typename T>
bool operator!=(const mat2<T>& lhs, const mat2<T>& rhs)
{
    return !(lhs == rhs);
}

typedef vec2<uint32> v2ui;
typedef vec2<int32>  v2i;
typedef vec2<float>  v2f;

typedef mat2<uint32> m2ui;
typedef mat2<int32>  m2i;
typedef mat2<float>  m2f;
