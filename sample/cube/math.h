#pragma once

#include <limits>
#include <cmath>
#include "shader/shader_common.h"

#define al_fequal(f1,f2) (abs(f1 - f2) < 1e-4)

#define infinity 1e10
#define infinity_i 1 << 30

namespace Math {
    inline bool isNan(float f)
    {
        union { float f; int i; } u;
        u.f = f;
        return (u.i & 0x7fffffff) > 0x7f800000;
    }

    inline constexpr float pi = 3.1415926;
    inline constexpr float eta = 1e-4;
}

struct Vector2f {
    union {
        struct {
            float x, y;
        };
        float a[2];
    };
    Vector2f(){
        memset(this, 0, sizeof(Vector2f));
    }
    Vector2f(const Vector2f& v) {
        memcpy(this, &v, sizeof(Vector2f));
    }
    Vector2f(const float* f) {
        x = f[0], y = f[1];
    }
    Vector2f(float x, float y);

    Vector2f(const vec2& v) {
        x = v.a[0], y = v.a[1];
    }

    operator vec2() 
    {
        return vec2{ x,y };
    }

    static Vector2f I;
};

struct Vector3i
{
	union {
		struct {
			int32_t x, y, z;
		};
		int32_t a[3];
	};

	Vector3i(int32_t x, int32_t y, int32_t z) :x(x), y(y), z(z) {}
};

struct Vector3f {
    union{
        struct {
            float x,y,z;
        };
        float a[3];
    };
    Vector3f(){
        memset(this, 0, sizeof(Vector3f));
    }
    Vector3f(const Vector3f& vec) {
        memcpy(this,&vec,sizeof(vec));
    }
    Vector3f(const float* f) {
        x = f[0], y = f[1], z = f[2];
    }
    Vector3f(const vec3& v) 
    {
        x = v.a[0], y = v.a[1], z = v.a[2];
    }

    Vector3f(float x, float y, float z);

    Vector3f(Vector3i v)
    {
        x = v.a[0], y = v.a[1], z = v.a[2];
    }

    const Vector3f& operator=(const Vector3f& vec) {
        memcpy(this, &vec, sizeof(vec));
        return *this;
    }

    operator vec3() 
    {
        return vec3{ x,y,z };
    }

    static Vector3f I;
    static Vector3f Up;
    static Vector3f Forward;
    static Vector3f Right;
};

struct Vector4f {
    union {
        struct {
            float x, y, z, w;
        };
        float a[4];
    };
    Vector4f() {
        memset(this, 0, sizeof(Vector4f));
    }
    Vector4f(const Vector4f& vec) {
        memcpy(this, &vec,sizeof(Vector4f));
    }
    Vector4f(const float* f) {
        x = f[0], y = f[1], z = f[2], w = f[3];
    }

    Vector4f(float x, float y, float z, float w);

    Vector4f(const vec4& v) 
    {
        x = v.a[0], y = v.a[1], z = v.a[2], w = v.a[3];
    }

    const Vector4f& operator=(const Vector4f& vec) {
        memcpy(this, &vec,sizeof(vec));
        return *this;
    }

    Vector3f XYZ() { return Vector3f(x, y, z); }

    operator vec4() 
    {
        return vec4{ x,y,z };
    }

    static Vector4f I;
};




struct Quaternion {
    Vector4f val;

    Quaternion(const Quaternion& o):val(o.val) {}
    /// <summary>
    /// create a quaternion axis-angle
    /// </summary>
    /// <param name="axis">the axis of the quaternion</param>
    /// <param name="angle">the angle the quaternion rotate around the axis(radian measure)</param>
    Quaternion(const Vector3f& axis, float angle);
    Quaternion():val(0.f,0.f,0.f,1.f) {}

    const Quaternion& operator=(const Quaternion& q) {
        val = q.val;
        return *this;
    }

    static Quaternion I;
};

struct Ray {
    Vector3f o, d;
    Vector3f invd;
    Ray():d(1,0,0),invd(1,infinity,infinity) {}
    Ray(const Vector3f& o, const Vector3f& d) :o(o),d(d){
        invd.x = 1.f / d.x, invd.y = 1.f / d.y, invd.z = 1.f / d.z;
    }
    const Ray& operator=(const Ray& r) {
        o = r.o, d = r.d,invd = r.invd;
        return *this;
    }
    void SetDirection(const Vector3f& d) {
        this->d = d;
        invd.x = 1.f / d.x, invd.y = 1.f / d.y, invd.z = 1.f / d.z;
    }
    void SetPosition(const Vector3f& o) {
        this->o = o;
    }
};



struct Mat4x4{
    union{
        struct {
            float a00, a01, a02, a03,
                  a10, a11, a12, a13,
                  a20, a21, a22, a23,
                  a30, a31, a32, a33;
        };
        float a[4][4];
        float raw[16];
    };
    Mat4x4() {
        memset(this, 0, sizeof(Vector4f));
    }
    Mat4x4(const Mat4x4& mat) {
        memcpy(this, &mat,sizeof(mat));
    }

    Mat4x4(float a00,float a01,float a02,float a03,
           float a10,float a11,float a12,float a13,
           float a20,float a21,float a22,float a23,
           float a30,float a31,float a32,float a33) {
        a[0][0] = a00, a[0][1] = a01, a[0][2] = a02, a[0][3] = a03;
        a[1][0] = a10, a[1][1] = a11, a[1][2] = a12, a[1][3] = a13;
        a[2][0] = a20, a[2][1] = a21, a[2][2] = a22, a[2][3] = a23;
        a[3][0] = a30, a[3][1] = a31, a[3][2] = a32, a[3][3] = a33;
    }

    Mat4x4(const mat4& m) 
    {
		a00 = m.a00, a01 = m.a01, a02 = m.a02, a03 = m.a03;
		a10 = m.a10, a11 = m.a11, a12 = m.a12, a13 = m.a13;
		a20 = m.a20, a21 = m.a21, a22 = m.a22, a23 = m.a23;
		a30 = m.a30, a31 = m.a31, a32 = m.a32, a33 = m.a33;
    }

    const Mat4x4& operator=(const Mat4x4& mat) {
        memcpy(this, &mat, sizeof(mat));
        return *this;
    }

    operator mat4() 
    {
        mat4 m;
        m.a00 = a00, m.a01 = a01, m.a02 = a02, m.a03 = a03;
		m.a10 = a10, m.a11 = a11, m.a12 = a12, m.a13 = a13;
		m.a20 = a20, m.a21 = a21, m.a22 = a22, m.a23 = a23;
		m.a30 = a30, m.a31 = a31, m.a32 = a32, m.a33 = a33;
        return m;
    }

    Vector4f row(uint32_t i) const {
        return Vector4f(a[i][0], a[i][1], a[i][2], a[i][3]);
    }

    Vector4f column(uint32_t i) const {
        return Vector4f(a[0][i], a[1][i], a[2][i], a[3][i]);
    }

    static Mat4x4 I;
};


struct Bound3f {
    union {
        struct {
            Vector3f lower, upper;
        };
        Vector3f bound[2];
    };
    Bound3f() {
        constexpr float fmin = -1e10;
        constexpr float fmax = 1e10;
        lower = Vector3f(fmax, fmax, fmax);
        upper = Vector3f(fmin, fmin, fmin);
    }

    Bound3f(Vector3f lower, Vector3f upper):lower(lower),
    upper(upper){}

    Bound3f(const Bound3f& b) {
        lower = b.lower, upper = b.upper;
    }

    const Bound3f& operator=(const Bound3f& b) {
        lower = b.lower, upper = b.upper;
        return *this;
    }
};

struct Transform {

    Transform() :quat(Vector3f(0., 1., 0.), 0.),scale(Vector3f::I) { RecomputeMatrix(); }
    Transform(const Vector3f& position, const Quaternion& quat, const Vector3f& scale) :
        position(position), quat(quat), scale(scale) {
        RecomputeMatrix();
    }
    Transform(const Transform& other):position(other.position),
        quat(other.quat),scale(other.scale),world(other.world),
        transInvWorld(other.transInvWorld){}
    
    const Transform& operator=(const Transform& other) {
        position = other.position;
        quat = other.quat;
        scale = other.scale;
        world = other.world;
        transInvWorld = other.transInvWorld;
        return *this;
    }

    void SetPosition(const Vector3f& position);
    void SetRotation(const Quaternion& quat);
    void SetScale(const Vector3f& scale);

    const Vector3f& GetPosition() const { return position; }
    const Quaternion& GetRotation() const { return quat; }
    const Vector3f& GetScale() const { return scale; }
    const Mat4x4& GetMatrix() const {return world;}
    const Mat4x4& GetTransInvMatrix() const { return transInvWorld; }

private:
    void RecomputeMatrix();

    Vector3f position;
    Quaternion quat;
    Vector3f scale;

    Mat4x4   world;
    Mat4x4   transInvWorld;
};

struct Intersection {
    float t;
    Vector2f uv;
    Vector2f localUv;
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    //position push a little forward out of the intersection point
    //this is used for ray casting to prevent self intersection
    Vector3f      adjustedPosition;
};


//TODO rewrite using concept in c++20
namespace Math {
    template<typename T>
    T clamp(T a,T lower,T upper) {
        return std::max(std::min(a, upper), lower);
    }

   
  
    float frac(float v);

    //from sin and cos to 0~2pi radius
    float angle(float sinValue,float cosValue);

    //   /---------
    // \/(1 - a * a)
    float safeSqrtOneMinusSq(float sinValue);

    Vector2f vmax(const Vector2f& a, const Vector2f& b);

    Vector2f vmin(const Vector2f& a, const Vector2f& b);

    Vector3f vmax(const Vector3f& a,const Vector3f& b);

    Vector3f vmin(const Vector3f& a,const Vector3f& b);

    Vector4f vmax(const Vector4f& a, const Vector4f& b);

    Vector4f vmin(const Vector4f& a, const Vector4f& b);

    template<>
    inline Vector2f clamp(Vector2f l, Vector2f lower, Vector2f upper) {
        return vmax(vmin(l, upper), lower);
    }
    template<>
    inline Vector3f clamp(Vector3f l, Vector3f lower, Vector3f upper) {
        return vmax(vmin(l, upper), lower);
    }
    template<>
    inline Vector4f clamp(Vector4f l, Vector4f lower, Vector4f upper) {
        return vmax(vmin(l, upper), lower);
    }

    float length(const Vector3f& vec);

    Vector3f normalize(const Vector3f& vec);

    float dot(const Vector3f& lhs,const Vector3f& rhs);

    Vector3f cross(const Vector3f& a,const Vector3f& b);

    Mat4x4 multiply(const Mat4x4& a,const Mat4x4& b);

    Vector3f transform_point(const Mat4x4& m,const Vector3f& pt);

    Vector3f transform_vector(const Mat4x4& m,const Vector3f& vc);

    Bound3f bound_merge(const Bound3f& a,const Bound3f& b);

    Bound3f bound_merge(const Bound3f& a,const Vector3f& v);

    Vector3f bound_centorid(const Bound3f& b);

    Mat4x4 transpose(const Mat4x4& m);

    Mat4x4 inverse(const Mat4x4& m);

    Mat4x4 perspective(float aspectRatio,float fov,float near,float far);

    Mat4x4 mat_position(const Vector3f& pos);

    Mat4x4 mat_rotation(const Quaternion& quat);

    Mat4x4 mat_scale(const Vector3f& scale);

    Mat4x4 mat_transform(const Vector3f& pos,const Quaternion& quat,const Vector3f& scale);

    //v0 * (1 - b) + v1 * b
    Vector2f interpolate2(const Vector2f& v0, const Vector2f& v1, float b);

    //v0 * (1 - b) + v1 * b
    Vector3f interpolate2(const Vector3f& v0, const Vector3f& v1, float b);
    
    //v0 * (1 - b) + v1 * b
    Vector4f interpolate2(const Vector4f& v0, const Vector4f& v1, float b);

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector2f interpolate3(const Vector2f& v0, const Vector2f& v1, const Vector2f& v2, const Vector2f& uv);

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector3f interpolate3(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector2f& uv);

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector4f interpolate3(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2, const Vector2f& uv);

    //intersect with triangle
    bool   ray_intersect_triangle(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3, const Ray& r,
         float* t, Vector2f* uv, Vector3f* position);

    //intersect with plane
    bool ray_intersect_plane(const Vector3f& dl,const Vector3f& dr,const Vector3f& ul, const Ray& r,
        float* t, Vector2f* uv, Vector3f* position);

    Vector3f color_blend(const Vector3f& c1, const Vector3f& c2);

    bool contains_nan(const Vector2f& v);
    bool contains_nan(const Vector3f& v);
    bool contains_nan(const Vector4f& v);
    
    bool contains_inf(const Vector2f& v);
    bool contains_inf(const Vector3f& v);
    bool contains_inf(const Vector4f& v);

    bool zero(const Vector3f& v);
};

Vector2f operator-(const Vector2f& a, const Vector2f& b);
Vector3f operator-(const Vector3f& a, const Vector3f& b);
Vector3i operator-(const Vector3i& a, const Vector3i& b);
Vector4f operator-(const Vector4f& a, const Vector4f& b);

Vector2f operator+(const Vector2f& a, const Vector2f& b);
Vector3f operator+(const Vector3f& a, const Vector3f& b);
Vector3i operator+(const Vector3i& a, const Vector3i& b);
Vector4f operator+(const Vector4f& a, const Vector4f& b);

Vector2f operator+(const Vector2f& a, float b);
Vector3f operator+(const Vector3f& a, float b);
Vector3i operator+(const Vector3i& a, int32_t b);
Vector4f operator+(const Vector4f& a, float b);

Vector2f operator+(float b, const Vector2f& a);
Vector3f operator+(float b, const Vector3f& a);
Vector3i operator+(int32_t b, const Vector3i& a);
Vector4f operator+(float b, const Vector4f& a);

Vector2f operator-(const Vector2f& a, float b);
Vector3f operator-(const Vector3f& a, float b);
Vector4f operator-(const Vector4f& a, float b);
Vector3i operator-(const Vector3i& a, int32_t b);

Vector2f operator-(float b, const Vector2f& a);
Vector3f operator-(float b, const Vector3f& a);
Vector4f operator-(float b, const Vector4f& a);

Vector2f operator*(const Vector2f& a, const Vector2f& b);
Vector3f operator*(const Vector3f& a, const Vector3f& b);
Vector4f operator*(const Vector4f& a, const Vector4f& b);

Vector2f operator/(const Vector2f& a, const Vector2f& b);
Vector3f operator/(const Vector3f& a, const Vector3f& b);
Vector4f operator/(const Vector4f& a, const Vector4f& b);

Vector2f operator*(const Vector2f& a, float b);
Vector3f operator*(const Vector3f& a, float b);
Vector4f operator*(const Vector4f& a, float b);

Vector2f operator/(const Vector2f& a, float b);
Vector3f operator/(const Vector3f& a, float b);
Vector4f operator/(const Vector4f& a, float b);

inline Vector2f operator*(float b, const Vector2f& a) { return a * b; }
inline Vector3f operator*(float b, const Vector3f& a) { return a * b; }
inline Vector4f operator*(float b, const Vector4f& a) { return a * b; }

Mat4x4 operator*(const Mat4x4& a, const Mat4x4& b);
Vector4f operator*(const Mat4x4& a, const Vector4f& b);
Vector4f operator*(const Vector4f& b,const Mat4x4& a);

