#pragma once

#include "../../Core/CoreDefines.h"

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

    const T& operator[](size_t index) const
    {
        TINKER_ASSERT(index < 2);
        return m_data[index];
    }

    T& operator[](size_t index)
    {
        TINKER_ASSERT(index < 2);
        return m_data[index];
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
    union
    {
        T m_data[4];
        struct
        {
            vec2<T> m_cols[2];
        };
    };
    
    mat2()
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = {};
    }

    mat2(const mat2<T>& other)
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = other.m_data[i];
    }

    mat2(const T* data)
    {
        for (uint8 i = 0; i < 4; ++i) m_data[i] = data[i];
    }

    mat2(const T& x)
    {
        // Initialize diagonal
        m_data[0] = x;
        m_data[1] = {};
        m_data[2] = {};
        m_data[3] = x;
    }

    mat2(const vec2<T>& a, const vec2<T>& b)
    {
        // Column-major
        m_data[0] = a.x; m_data[2] = b.x;
        m_data[1] = a.y; m_data[3] = b.y;
    }

    mat2(const T& x00, const T& x01, const T& x10, const T& x11)
    {
        // Column-major
        m_data[0] = x00; m_data[2] = x10;
        m_data[1] = x01; m_data[3] = x11;
    }

    const vec2<T>& operator[](size_t index) const
    {
        TINKER_ASSERT(index < 2);
        return m_cols[index];
    }

    vec2<T>& operator[](size_t index)
    {
        TINKER_ASSERT(index < 2);
        return m_cols[index];
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
vec2<T> operator*(const mat2<T>& m, const vec2<T>& v)
{
    return { m[0][0] * v.x + m[1][0] * v.y, m[0][1] * v.x + m[1][1] * v.y };
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

template <typename T>
class vec4
{
public:
    union
    {
        T m_data[4];
        struct
        {
            T x;
            T y;
            T z;
            T w;
        };
    };

    vec4()
    {
        x = {};
        y = {};
        z = {};
        w = {};
    }

    vec4(const vec4& v)
    {
        this->x = v.x;
        this->y = v.y;
        this->z = v.z;
        this->w = v.w;
    }

    vec4(const T* data)
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            m_data[i] = data[i];
        }
    }

    vec4(const T& x)
    {
        this->x = x;
        this->y = x;
        this->z = x;
        this->w = x;
    }

    vec4(const T& x, const T& y, const T& z, const T& w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    const T& operator[](size_t index) const
    {
        TINKER_ASSERT(index < 4);
        return m_data[index];
    }

    T& operator[](size_t index)
    {
        TINKER_ASSERT(index < 4);
        return m_data[index];
    }

    vec4<T> operator+(const vec4<T>& other)
    {
        return { x + other.x, y + other.y, z + other.z, w + other.w };
    }

    vec4<T> operator-(const vec4<T>& other)
    {
        return { x - other.x, y - other.y, z - other.z, w - other.w };
    }

    vec4<T> operator*(const vec4<T>& other)
    {
        return { x * other.x, y * other.y, z * other.z, w * other.w };
    }

    vec4<T> operator/(const vec4<T>& other)
    {
        return { x / other.x, y / other.y, z / other.z, w / other.w };
    }

    void operator+=(const vec4<T>& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
    }

    void operator-=(const vec4<T>& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
    }

    void operator*=(const vec4<T>& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
    }

    void operator/=(const vec4<T>& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
    }
};

template <typename T>
bool operator==(const vec4<T>& lhs, const vec4<T>& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

template <typename T>
bool operator!=(const vec4<T>& lhs, const vec4<T>& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
class mat4
{
public:
    union
    {
        T m_data[16];
        struct
        {
            vec4<T> m_cols[4];
        };
    };

    mat4()
    {
        for (uint8 i = 0; i < 16; ++i) m_data[i] = {};
    }

    mat4(const mat4<T>& other)
    {
        for (uint8 i = 0; i < 16; ++i) m_data[i] = other.m_data[i];
    }

    mat4(const T* data)
    {
        for (uint8 i = 0; i < 16; ++i) m_data[i] = data[i];
    }

    mat4(const T& x)
    {
        // Initialize diagonal
        m_data[0] = x;
        m_data[1] = {};
        m_data[2] = {};
        m_data[3] = {};

        m_data[4] = {};
        m_data[5] = x;
        m_data[6] = {};
        m_data[7] = {};

        m_data[8] = {};
        m_data[9] = {};
        m_data[10] = x;
        m_data[11] = {};

        m_data[12] = {};
        m_data[13] = {};
        m_data[14] = {};
        m_data[15] = x;
    }

    mat4(const vec4<T>& a, const vec4<T>& b, const vec4<T>& c, const vec4<T>& d)
    {
        // Column-major
        m_data[0] = a.x; m_data[4] = b.x; m_data[8]  = c.x; m_data[12] = d.x;
        m_data[1] = a.y; m_data[5] = b.y; m_data[9]  = c.y; m_data[13] = d.y;
        m_data[2] = a.z; m_data[6] = b.x; m_data[10] = c.z; m_data[14] = d.x;
        m_data[3] = a.w; m_data[7] = b.y; m_data[11] = c.w; m_data[15] = d.y;
    }

    mat4(const T& x00, const T& x01, const T& x02, const T& x03,
         const T& x10, const T& x11, const T& x12, const T& x13,
         const T& x20, const T& x21, const T& x22, const T& x23,
         const T& x30, const T& x31, const T& x32, const T& x33)
    {
        // Column-major
        m_data[0] = x00; m_data[4] = x10; m_data[8]  = x20; m_data[12] = x30;
        m_data[1] = x01; m_data[5] = x11; m_data[9]  = x21; m_data[13] = x31;
        m_data[2] = x02; m_data[6] = x12; m_data[10] = x22; m_data[14] = x32;
        m_data[3] = x03; m_data[7] = x13; m_data[11] = x23; m_data[15] = x33;
    }

    const vec4<T>& operator[](size_t index) const
    {
        TINKER_ASSERT(index < 4);
        return m_cols[index];
    }

    vec4<T>& operator[](size_t index)
    {
        TINKER_ASSERT(index < 4);
        return m_cols[index];
    }

    mat4<T> operator+(const mat4<T>& other)
    {
        T newData[16];
        for (uint8 i = 0; i < 16; ++i)
        {
            newData[i] = m_data[i] + other.m_data[i];
        }
        return { newData };
    }

    mat4<T> operator-(const mat4<T>& other)
    {
        T newData[16];
        for (uint8 i = 0; i < 16; ++i)
        {
            newData[i] = m_data[i] - other.m_data[i];
        }
        return { newData };
    }

    mat4<T> operator*(const mat4<T>& other)
    {
        T newData[16];
        for (uint8 i = 0; i < 16; ++i)
        {
            newData[i] = m_data[i] * other.m_data[i];
        }
        return { newData };
    }

    mat4<T> operator/(const mat4<T>& other)
    {
        T newData[16];
        for (uint8 i = 0; i < 16; ++i)
        {
            newData[i] = m_data[i] / other.m_data[i];
        }
        return { newData };
    }

    void operator+=(const mat4<T>& other)
    {
        for (uint8 i = 0; i < 16; ++i)
        {
            m_data[i] += other.m_data[i];
        }
    }

    void operator-=(const mat4<T>& other)
    {
        for (uint8 i = 0; i < 16; ++i)
        {
            m_data[i] -= other.m_data[i];
        }
    }

    void operator*=(const mat4<T>& other)
    {
        for (uint8 i = 0; i < 16; ++i)
        {
            m_data[i] *= other.m_data[i];
        }
    }

    void operator/=(const mat4<T>& other)
    {
        for (uint8 i = 0; i < 16; ++i)
        {
            m_data[i] /= other.m_data[i];
        }
    }
};

template <typename T>
vec4<T> operator*(const mat4<T>& m, const vec4<T>& v)
{
    return {
        m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
        m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w, 
        m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w, 
        m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w
    };
}

template <typename T>
bool operator==(const mat4<T>& lhs, const mat4<T>& rhs)
{
    bool equal = true;
    for (uint8 i = 0; i < 16; ++i)
    {
        equal &= lhs.m_data[i] == rhs.m_data[i];
    }
    return equal;
}

template <typename T>
bool operator!=(const mat4<T>& lhs, const mat4<T>& rhs)
{
    return !(lhs == rhs);
}

typedef vec2<uint32> v2ui;
typedef vec2<int32>  v2i;
typedef vec2<float>  v2f;
typedef vec4<uint32> v4ui;
typedef vec4<int32>  v4i;
typedef vec4<float>  v4f;

typedef mat2<uint32> m2ui;
typedef mat2<int32>  m2i;
typedef mat2<float>  m2f;
typedef mat4<uint32> m4ui;
typedef mat4<int32>  m4i;
typedef mat4<float>  m4f;

namespace VectorOps
{
    v2f Mul_SIMD(const v2f& v, const m2f& m);
    v2i Mul_SIMD(const v2i& v, const m2i& m);
    v2ui Mul_SIMD(const v2ui& v, const m2ui& m);

    void Mul_SIMD(const v4f& v, const m4f& m, v4f& out);
    v4i Mul_SIMD(const v4i& v, const m4i& m);
    v4ui Mul_SIMD(const v4ui& v, const m4ui& m);
}
