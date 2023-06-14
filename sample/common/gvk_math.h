#pragma once

#include <limits>
#include <cmath>
#include "shader/shader_common.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;

using Vector2i = glm::ivec2;
using Vector3i = glm::ivec3;
using Vector4i = glm::ivec4;

using Mat4x4 = glm::mat4;


struct Quaternion
{
	glm::quat quaternion;

	Quaternion(float angle, Vector3f v)
	{
		quaternion = glm::angleAxis(angle, v);
	}

	Quaternion(Vector3f euler)
	{
		quaternion = glm::quat(euler);
	}

	Quaternion(const Quaternion& q)
	{
		quaternion = q.quaternion;
	}

	Mat4x4 GetMatrix() const
	{
		return glm::mat4_cast(quaternion);
	}
};


struct Transform {

	Transform() :quat(0.f, Vector3f(0.f, 1.f, 0.f)), scale(Vector3f(1.0f)) { RecomputeMatrix(); }
	Transform(const Vector3f& position, const Quaternion& quat, const Vector3f& scale) :
		position(position), quat(quat), scale(scale) {
		RecomputeMatrix();
	}
	Transform(const Transform& other) :position(other.position),
		quat(other.quat), scale(other.scale), world(other.world),
		transInvWorld(other.transInvWorld) {}

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
	void SetRotation(const Vector3f& euler);
	void SetScale(const Vector3f& scale);

	const Vector3f& GetPosition() const { return position; }
	const Quaternion& GetRotation() const { return quat; }
	const Vector3f& GetScale() const { return scale; }
	const Mat4x4& GetMatrix() const { return world; }
	const Mat4x4& GetTransInvMatrix() const { return transInvWorld; }

private:
	void RecomputeMatrix();

	Vector3f position;
	Quaternion quat;
	Vector3f scale;

	Mat4x4   world;
	Mat4x4   transInvWorld;
};



namespace Math
{
	constexpr float pi = 3.1415926;

	float radians(float degree);

	Vector3f normalize(Vector3f vec);

	Vector3f cross(Vector3f v1, Vector3f v2);

	float	 dot(Vector3f v1, Vector3f v2);

	Mat4x4 perspective(float fov, float ratio, float near, float far);

	Mat4x4 position(const Vector3f& offset);

	Mat4x4 rotation(float angle, const Vector3f& v);

	Mat4x4 rotation(const Quaternion& quat);

	Mat4x4 scale(const Vector3f& s);

	Mat4x4 transpose(const Mat4x4& m);

	Mat4x4 inverse(const Mat4x4& m);


	template<typename T>
	T lerp(const T& a, const T& b, float f)
	{
		return a * (1 - f) + b * f;
	}

	Mat4x4	look_at(Vector3f pos, Vector3f center, Vector3f up);

	float	distance(Vector2f lhs, Vector2f rhs);

	float	distance(Vector3f lhs, Vector3f rhs);

	float	distance(Vector4f lhs, Vector4f rhs);
}

