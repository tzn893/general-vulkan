#include "math.h"

namespace Math {
    
    float frac(float v) {
        return v - (float)((uint32_t)v);
    }

    Vector3f vmax(const Vector3f& a,const Vector3f& b) {
        return Vector3f(a.x > b.x ? a.x : b.x,
                        a.y > b.y ? a.y : b.y,
                        a.z > b.z ? a.z : b.z);
    }

    Vector3f vmin(const Vector3f& a,const Vector3f& b) {
        return Vector3f(a.x < b.x ? a.x : b.x,
                        a.y < b.y ? a.y : b.y,
                        a.z < b.z ? a.z : b.z);
    }


    float length(const Vector3f& vec) {
        //TODO: can be optimized
        return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    }

    Vector3f normalize(const Vector3f& vec) {
        float len = length(vec);
        return Vector3f(vec.x / len, vec.y / len, vec.z / len);
    }

    float dot(const Vector3f& lhs,const Vector3f& rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    Vector3f cross(const Vector3f& a,const Vector3f& b) {
        return Vector3f(a.y * b.z - a.z * b.y,
                        a.z * b.x - a.x * b.z,
                        a.x * b.y - a.y * b.x);
    }

    Mat4x4 multiply(const Mat4x4& a,const Mat4x4& b) {
        float r[4][4];
        r[0][0] = a.a[0][0] * b.a[0][0] + a.a[0][1] * b.a[1][0] + a.a[0][2] * b.a[2][0] + a.a[0][3] * b.a[3][0];
        r[0][1] = a.a[0][0] * b.a[0][1] + a.a[0][1] * b.a[1][1] + a.a[0][2] * b.a[2][1] + a.a[0][3] * b.a[3][1];
        r[0][2] = a.a[0][0] * b.a[0][2] + a.a[0][1] * b.a[1][2] + a.a[0][2] * b.a[2][2] + a.a[0][3] * b.a[3][2];
        r[0][3] = a.a[0][0] * b.a[0][3] + a.a[0][1] * b.a[1][3] + a.a[0][2] * b.a[2][3] + a.a[0][3] * b.a[3][3];


        r[1][0] = a.a[1][0] * b.a[0][0] + a.a[1][1] * b.a[1][0] + a.a[1][2] * b.a[2][0] + a.a[1][3] * b.a[3][0];
        r[1][1] = a.a[1][0] * b.a[0][1] + a.a[1][1] * b.a[1][1] + a.a[1][2] * b.a[2][1] + a.a[1][3] * b.a[3][1];
        r[1][2] = a.a[1][0] * b.a[0][2] + a.a[1][1] * b.a[1][2] + a.a[1][2] * b.a[2][2] + a.a[1][3] * b.a[3][2];
        r[1][3] = a.a[1][0] * b.a[0][3] + a.a[1][1] * b.a[1][3] + a.a[1][2] * b.a[2][3] + a.a[1][3] * b.a[3][3];

        r[2][0] = a.a[2][0] * b.a[0][0] + a.a[2][1] * b.a[1][0] + a.a[2][2] * b.a[2][0] + a.a[2][3] * b.a[3][0];
        r[2][1] = a.a[2][0] * b.a[0][1] + a.a[2][1] * b.a[1][1] + a.a[2][2] * b.a[2][1] + a.a[2][3] * b.a[3][1];
        r[2][2] = a.a[2][0] * b.a[0][2] + a.a[2][1] * b.a[1][2] + a.a[2][2] * b.a[2][2] + a.a[2][3] * b.a[3][2];
        r[2][3] = a.a[2][0] * b.a[0][3] + a.a[2][1] * b.a[1][3] + a.a[2][2] * b.a[2][3] + a.a[2][3] * b.a[3][3];

        r[3][0] = a.a[3][0] * b.a[0][0] + a.a[3][1] * b.a[1][0] + a.a[3][2] * b.a[2][0] + a.a[3][3] * b.a[3][0];
        r[3][1] = a.a[3][0] * b.a[0][1] + a.a[3][1] * b.a[1][1] + a.a[3][2] * b.a[2][1] + a.a[3][3] * b.a[3][1];
        r[3][2] = a.a[3][0] * b.a[0][2] + a.a[3][1] * b.a[1][2] + a.a[3][2] * b.a[2][2] + a.a[3][3] * b.a[3][2];
        r[3][3] = a.a[3][0] * b.a[0][3] + a.a[3][1] * b.a[1][3] + a.a[3][2] * b.a[2][3] + a.a[3][3] * b.a[3][3];

        return Mat4x4(r[0][0], r[0][1], r[0][2], r[0][3],
                      r[1][0], r[1][1], r[1][2], r[1][3], 
                      r[2][0], r[2][1], r[2][2], r[2][3], 
                      r[3][0], r[3][1], r[3][2], r[3][3]);
    }

    Vector3f transform_point(const Mat4x4& m,const Vector3f& pt) {
        constexpr float p = 1.0f;
        float r[3];
        r[0] = m.a[0][0] * pt.x + m.a[0][1] * pt.y + m.a[0][2] * pt.z + m.a[0][3] * p;
        r[1] = m.a[1][0] * pt.x + m.a[1][1] * pt.y + m.a[1][2] * pt.z + m.a[1][3] * p;
        r[2] = m.a[2][0] * pt.x + m.a[2][1] * pt.y + m.a[2][2] * pt.z + m.a[2][3] * p;
        return Vector3f(r[0], r[1], r[2]);
    }

    Vector3f transform_vector(const Mat4x4& m,const Vector3f& vc) {
        constexpr float p = 0.0f;
        float r[3];
        r[0] = m.a[0][0] * vc.x + m.a[0][1] * vc.y + m.a[0][2] * vc.z + m.a[0][3] * p;
        r[1] = m.a[1][0] * vc.x + m.a[1][1] * vc.y + m.a[1][2] * vc.z + m.a[1][3] * p;
        r[2] = m.a[2][0] * vc.x + m.a[2][1] * vc.y + m.a[2][2] * vc.z + m.a[2][3] * p;
        return Vector3f(r[0], r[1], r[2]);
    }

    Bound3f bound_merge(const Bound3f& a,const Bound3f& b) {
        
        return Bound3f(vmin(a.lower, b.lower), vmax(a.upper, b.upper));
    }

    Bound3f bound_merge(const Bound3f& a,const Vector3f& v) {
        return Bound3f(vmin(a.lower, v), vmax(a.upper, v));
    }

    Mat4x4 transpose(const Mat4x4& m) {
        return Mat4x4(m.a00, m.a10, m.a20, m.a30,
                      m.a01, m.a11, m.a21, m.a31, 
                      m.a02, m.a12, m.a22, m.a32, 
                      m.a03, m.a13, m.a23, m.a33);
    }

    //code from https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/matrix-inverse
    Mat4x4 inverse(const Mat4x4& _m) {
        Mat4x4 mat(1.f,0.f,0.f,0.f,
                   0.f,1.f,0.f,0.f,
                   0.f,0.f,1.f,0.f,
                   0.f,0.f,0.f,1.f
        );
        Mat4x4 m = _m;

        for (unsigned column = 0; column < 4; ++column) {
            // Swap row in case our pivot point is not working
            if (m.a[column][column] == 0) {
                unsigned big = column;
                for (unsigned row = 0; row < 4; ++row)
                    if (fabs(m.a[row][column]) > fabs(m.a[big][column])) big = row;
                // Print this is a singular matrix, return identity ?
                // TODO error handling
                // Swap rows
                for (unsigned j = 0; j < 4; ++j) {
                    std::swap(m.a[column][j], m.a[big][j]);
                    std::swap(mat.a[column][j], mat.a[big][j]);
                }
            }
            // Set each row in the column to 0  
            for (unsigned row = 0; row < 4; ++row) {
                if (row != column) {
                    float coeff = m.a[row][column] / m.a[column][column];
                    if (coeff != 0) {
                        for (unsigned j = 0; j < 4; ++j) {
                            m.a[row][j] -= coeff * m.a[column][j];
                            mat.a[row][j] -= coeff * mat.a[column][j];
                        }
                        // Set the element to 0 for safety
                        m.a[row][column] = 0;
                    }
                }
            }
        }
        // Set each element of the diagonal to 1
        for (unsigned row = 0; row < 4; ++row) {
            for (unsigned column = 0; column < 4; ++column) {
                mat.a[row][column] /= m.a[row][row];
            }
        }

        return mat;
    }

    Mat4x4 perspective(float aspectRatio, float fov, float near, float far) {
		float ta = tanf(fov * .5f);
		return Mat4x4(
			1.f / (aspectRatio * ta), 0.0f, 0.0f, 0.0f,
			0.0f, 1.f / ta, 0.0f, 0.0f,
			0.0f, 0.0f,  far / (near - far), -near * far / (near - far),
			0.0f, 0.0f, -1,  0.0f
		);
    }

    Mat4x4 mat_position(const Vector3f& pos) {
        return Mat4x4(
            1.0f, 0.0f, 0.0f, pos.x,
            0.0f, 1.0f, 0.0f, pos.y,
            0.0f, 0.0f, 1.0f, pos.z,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Mat4x4 mat_rotation(const Quaternion& quat) {
        float x = quat.val.x, y = quat.val.y, z = quat.val.z,
            w = quat.val.w;
        float x2 = x * x, y2 = y * y, z2 = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = x * w, wy = w * y, wz = w * z;
        return Mat4x4(
            1 - 2 * (y2 + z2)   , 2 * (xy - wz)     , 2 * (xz + wy)     , 0,
            2 * (xy + wz)       , 1 - 2 * (x2 + z2) , 2 * (yz - wx)     , 0,
            2 * (xz - wy)       , 2 * (yz + wx)     , 1 - 2 * (x2 + y2) , 0,
            0                   , 0                 , 0                 , 1
        );
    }

    Mat4x4 mat_scale(const Vector3f& scale) {
        return Mat4x4(
            scale.x , 0         , 0         , 1,
            0       , scale.y   , 0         , 1,
            0       , 0         , scale.z   , 1,
            0       , 0         ,0          , 1
        );
    }

    Mat4x4 mat_transform(const Vector3f& pos, const Quaternion& quat, const Vector3f& scale) {
        Mat4x4 res = mat_rotation(quat);
        res.a00 *= scale.x, res.a01 *= scale.x, res.a02 *= scale.x,res.a03 = pos.x;
        res.a10 *= scale.y, res.a11 *= scale.y, res.a12 *= scale.y,res.a13 = pos.y;
        res.a20 *= scale.z, res.a21 *= scale.z, res.a22 *= scale.z,res.a23 = pos.z;
        return res;
    }

    Vector3f bound_centorid(const Bound3f& b) {
        float x = (b.upper.x + b.lower.x) * .5f;
        float y = (b.upper.y + b.lower.y) * .5f;
        float z = (b.upper.z + b.lower.z) * .5f;
        return Vector3f(x, y, z);
    }

    //v0 * (1 - b) + v1 * b
    Vector2f interpolate2(const Vector2f& v0, const Vector2f& v1, float b) {
        return (v0 * (1.f - b)) + (v1 * b);
    }

    //v0 * (1 - b) + v1 * b
    Vector3f interpolate2(const Vector3f& v0, const Vector3f& v1, float b) {
        return (v0 * (1.f - b)) + (v1 * b);
    }

    //v0 * (1 - b) + v1 * b
    Vector4f interpolate2(const Vector4f& v0, const Vector4f& v1, float b) {
        return (v0 * (1.f - b)) + (v1 * b);
    }

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector2f interpolate3(const Vector2f& v0, const Vector2f& v1, const Vector2f& v2, const Vector2f& uv) {
        return v0 * (1.f - uv.x - uv.y) + (v1 * uv.x) + (v2 * uv.y);
    }

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector3f interpolate3(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector2f& uv) {
        return v0 * (1.f - uv.x - uv.y) + (v1 * uv.x) + (v2 * uv.y);
    }

    //v0 * (1 - u -  v) + v1 * u + v2 * v
    Vector4f interpolate3(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2, const Vector2f& uv) {
        return v0 * (1.f - uv.x - uv.y) + (v1 * uv.x) + (v2 * uv.y);
    }




    bool   ray_intersect_triangle(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Ray& r,
        float* t, Vector2f* uv, Vector3f* position) {
       
        //from paper https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
        Vector3f e1 = v1 - v0;
        Vector3f e2 = v2 - v0;
        Vector3f o = r.o - v0;

        Vector3f cross_d_e2 = Math::cross(r.d, e2);
        float det = Math::dot(cross_d_e2, e1);

        if (al_fequal(det, 0)) {
            return false;
        }
        float invDet = 1.f / det;
        float u, v;
        u = Math::dot(cross_d_e2, o) * invDet;
        if (u < 0.f || u > 1.f) return false;

        Vector3f cross_o_e1 = Math::cross(o, e1);
        v = Math::dot(cross_o_e1, r.d) * invDet;
        if (v < 0.f || (u + v) > 1.f) return false;

        *t = Math::dot(cross_o_e1, e2) * invDet;
        *uv = Vector2f(u, v);
        *position = *t * r.d + r.o;

        return *t > 0.f;  //this ray hits the triangle 
    }

    //double face plane intersection 
    bool ray_intersect_plane(const Vector3f& v0/*ul*/, const Vector3f& v1/*dr*/, const Vector3f& v2/*dl*/,
        const Ray& r,float* t, Vector2f* uv, Vector3f* position) {

        //copied from triangle intersection
        //only thing we have to do is just modify the last if condition from (u + v) > 1.f to v > 1.f
        Vector3f e1 = v1 - v0;
        Vector3f e2 = v2 - v0;
        Vector3f o = r.o - v0;

        Vector3f cross_d_e2 = Math::cross(r.d, e2);
        float det = Math::dot(cross_d_e2, e1);

        if (al_fequal(det, 0)) {
            return false;
        }
        float invDet = 1.f / det;
        float u, v;
        u = Math::dot(cross_d_e2, o) * invDet;
        if (u < 0.f || u > 1.f) return false;

        Vector3f cross_o_e1 = Math::cross(o, e1);
        v = Math::dot(cross_o_e1, r.d) * invDet;
        if (v < 0.f || v > 1.f) return false;

        *t = Math::dot(cross_o_e1, e2) * invDet;
        *uv = Vector2f(u, v);
        *position = *t * r.d + r.o;

        return *t > 0.f;  //this ray hits the plane 
    }


    bool contains_nan(const Vector2f& v) {
        return (isNan(v.x) || isNan(v.y));
    }

    bool contains_nan(const Vector3f& v) {
        return (isNan(v.x) || isNan(v.y) || isNan(v.z));
    }
    bool contains_nan(const Vector4f& v) {
        return (isNan(v.x) || isNan(v.y) || isNan(v.z) || isNan(v.w));
    }

    bool contains_inf(const Vector2f& v) {
        return std::isinf(v.x) || std::isinf(v.y) || 
            al_fequal(v.x, infinity) || al_fequal(v.y, infinity);
    }
    bool contains_inf(const Vector3f& v) {
        return std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.z)
            || al_fequal(v.x, infinity) || al_fequal(v.y, infinity) || al_fequal(v.z, infinity);
    }
    bool contains_inf(const Vector4f& v) {
        return std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.z) || std::isinf(v.w)
            || al_fequal(v.x, infinity) || al_fequal(v.y, infinity) || al_fequal(v.z, infinity) || al_fequal(v.w, infinity);
    }

    Vector3f color_blend(const Vector3f& c1, const Vector3f& c2) {
        return Vector3f(c1.x * c2.x, c1.y * c2.y, c1.z * c2.z);
    }

    float angle(float sinValue, float cosValue) {
        float v = atan2f(sinValue, cosValue);
        if (v < 0.f) {
            v += 2 * pi;
        }
        return v;
    }


    bool zero(const Vector3f& v) {
        return al_fequal(v.x, 0.f) && al_fequal(v.y, 0.f)
            && al_fequal(v.z, 0.f);
    }



    Vector2f vmax(const Vector2f& a, const Vector2f& b) {
        return Vector2f(a.x > b.x ? a.x : b.x,
            a.y > b.y ? a.y : b.y);
    }

    Vector2f vmin(const Vector2f& a, const Vector2f& b) {
        return Vector2f(a.x < b.x ? a.x : b.x,
            a.y < b.y ? a.y : b.y);
    }

    Vector4f vmax(const Vector4f& a, const Vector4f& b) {
        return Vector4f(a.x > b.x ? a.x : b.x,
            a.y > b.y ? a.y : b.y,
            a.z > b.z ? a.z : b.z,
            a.w > b.w ? a.w : b.w);
    }

    Vector4f vmin(const Vector4f& a, const Vector4f& b) {
        return Vector4f(a.x < b.x ? a.x : b.x,
            a.y < b.y ? a.y : b.y,
            a.z < b.z ? a.z : b.z,
            a.w < b.w ? a.w : b.w);
    }


};

Quaternion::Quaternion(const Vector3f& axis, float angle) {
    Vector3f n = Math::normalize(axis);
    float halfAngle = angle * .5f;
    float shAngle = sin(halfAngle), chAngle = cos(halfAngle);
    val.x = axis.x * shAngle;
    val.y = axis.y * shAngle;
    val.z = axis.z * shAngle;
    val.w = chAngle;
}

void Transform::SetPosition(const Vector3f& position) {
    this->position = position;
    RecomputeMatrix();
}
void Transform::SetRotation(const Quaternion& quat) {
    this->quat = quat;
    RecomputeMatrix();
}

void Transform::SetScale(const Vector3f& scale) {
    this->scale = scale;
    RecomputeMatrix();
}

void Transform::RecomputeMatrix() {
    world = Math::mat_transform(position, quat, scale);

    transInvWorld = Math::transpose(Math::inverse(world));
}


Vector3f operator-(const Vector3f& a, const Vector3f& b) {
    return Vector3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vector3i operator-(const Vector3i& a, int32_t b)
{
    return Vector3i(a.x - b, a.y - b, a.z - b);
}

Vector3i operator-(const Vector3i& a, const Vector3i& b)
{
	return Vector3i(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vector3f operator+(const Vector3f& a, const Vector3f& b) {
    return Vector3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vector3i operator+(const Vector3i& a, int32_t b)
{
	return Vector3i(a.x + b, a.y + b, a.z + b);
}

Vector3i operator+(int32_t b, const Vector3i& a)
{
	return Vector3i(a.x + b, a.y + b, a.z + b);
}

Vector3i operator+(const Vector3i& a, const Vector3i& b)
{
    return Vector3i(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vector3f operator*(const Vector3f& a, float f) {
    return Vector3f(a.x * f, a.y * f, a.z * f);
}

Vector3f operator/(const Vector3f& a, float f) {
    return Vector3f(a.x / f, a.y / f, a.z / f);
}

Vector2f operator-(const Vector2f& a, const Vector2f& b) {
    return Vector2f(a.x - b.x, a.y - b.y);
}

Vector2f operator+(const Vector2f& a, const Vector2f& b) {
    return Vector2f(a.x + b.x, a.y + b.y);
}

Vector2f operator*(const Vector2f& a, float f) {
    return Vector2f(a.x * f, a.y * f);
}

Vector2f operator/(const Vector2f& a, float f) {
    return Vector2f(a.x / f, a.y / f);
}

Vector4f operator-(const Vector4f& a, const Vector4f& b) {
    return Vector4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Vector4f operator+(const Vector4f& a, const Vector4f& b) {
    return Vector4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vector4f operator*(const Vector4f& a, float f) {
    return Vector4f(a.x * f, a.y * f, a.z * f, a.w * f);
}

Vector4f operator/(const Vector4f& a, float f) {
    return Vector4f(a.x / f, a.y / f, a.z / f, a.w / f);
}

Vector2f operator*(const Vector2f& a, const Vector2f& b) {
    return Vector2f(a.x * b.x, a.y * b.y);
}
Vector3f operator*(const Vector3f& a, const Vector3f& b) {
    return Vector3f(a.x * b.x, a.y * b.y, a.z * b.z);
}
Vector4f operator*(const Vector4f& a, const Vector4f& b) {
    return Vector4f(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Vector2f operator/(const Vector2f& a, const Vector2f& b) {
    return Vector2f(a.x / b.x, a.y / b.y);
}
Vector3f operator/(const Vector3f& a, const Vector3f& b) {
    return Vector3f(a.x / b.x, a.y / b.y, a.z / b.z);
}
Vector4f operator/(const Vector4f& a, const Vector4f& b) {
    return Vector4f(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}


Mat4x4 operator*(const Mat4x4& a, const Mat4x4& b) {
    return Math::multiply(a, b);
}
Vector4f operator*(const Mat4x4& a, const Vector4f& b) {
    return Vector4f(a.a00 * b.x + a.a01 * b.y + a.a02 * b.z + a.a03 * b.w,
        a.a10 * b.x + a.a11 * b.y + a.a12 * b.z + a.a13 * b.w,
        a.a20 * b.x + a.a21 * b.y + a.a22 * b.z + a.a23 * b.w,
        a.a30 * b.x + a.a31 * b.y + a.a32 * b.z + a.a33 * b.w);
}

Vector4f operator*(const Vector4f& b,const Mat4x4& a) {
    return Vector4f(a.a00 * b.x + a.a10 * b.y + a.a20 * b.z + a.a30 * b.w,
        a.a01 * b.x + a.a11 * b.y + a.a21 * b.z + a.a31 * b.w,
        a.a02 * b.x + a.a12 * b.y + a.a22 * b.z + a.a32 * b.w,
        a.a03 * b.x + a.a13 * b.y + a.a23 * b.z + a.a33 * b.w);
}


Vector2f Vector2f::I = Vector2f(1.f, 1.f);
Vector3f Vector3f::I = Vector3f(1.f, 1.f, 1.f);
Vector3f Vector3f::Up = Vector3f(0.f, 1.f, 0.f);
Vector3f Vector3f::Forward = Vector3f(0.f, 0.f, 1.f);
Vector3f Vector3f::Right = Vector3f(1.f, 0.f, 0.f);
Vector4f Vector4f::I = Vector4f(1.f, 1.f, 1.f, 1.f);

Quaternion Quaternion::I = Quaternion();
Mat4x4 Mat4x4::I = Mat4x4(
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
);

Vector3f::Vector3f(float x, float y, float z) :x(x), y(y), z(z) {
}

Vector2f::Vector2f(float x, float y) :x(x), y(y) {
}

Vector4f::Vector4f(float x, float y, float z,float w) :x(x), y(y), z(z),w(w) {
}


float Math::safeSqrtOneMinusSq(float sinValue) {
    float s = clamp(sinValue, -.9999f, .9999f);
    return sqrtf(1 - s * s);
}

Vector2f operator+(const Vector2f& a, float b) {
    return Vector2f(a.x + b, a.y + b);
}
Vector3f operator+(const Vector3f& a, float b) {
    return Vector3f(a.x + b, a.y + b, a.z + b);
}
Vector4f operator+(const Vector4f& a, float b) {
    return Vector4f(a.x + b, a.y + b, a.z + b, a.w + b);
}

Vector2f operator+(float b, const Vector2f& a) {
    return Vector2f(b + a.x, b + a.y);
}
Vector3f operator+(float b, const Vector3f& a) {
    return Vector3f(b + a.x, b + a.y, b + a.z);
}
Vector4f operator+(float b, const Vector4f& a) {
    return Vector4f(b + a.x, b + a.y, b + a.z, b + a.w);
}

Vector2f operator-(const Vector2f& a, float b) {
    return Vector2f(a.x - b, a.y - b);
}
Vector3f operator-(const Vector3f& a, float b) {
    return Vector3f(a.x - b, a.y - b, a.z - b);
}
Vector4f operator-(const Vector4f& a, float b) {
    return Vector4f(a.x - b, a.y - b, a.z - b, a.w - b);
}

Vector2f operator-(float b, const Vector2f& a) {
    return Vector2f(b - a.x, b - a.y);
}
Vector3f operator-(float b, const Vector3f& a) {
    return Vector3f(b - a.x, b - a.y, b - a.z);
}
Vector4f operator-(float b, const Vector4f& a) {
    return Vector4f(b - a.x, b - a.y, b - a.z, b - a.w);
}
