#pragma once
#include "PhysicsShapes.h"

#include "src/Core/Core.h"
#include "src/Core/Maths.h"
#include "src/Scene/Entities/Entity.h"

#include <glm/ext/vector_float3.hpp>

namespace Nuake
{
	class Entity;
	namespace Physics 
	{
		class RigidBody 
		{
		private:
			Ref<PhysicShape> _collisionShape;
			Vector3 _position;
			Quat _rotation;
			Entity _entity;

			bool m_LockXAxis = false;
			bool m_LockYAxis = false;
			bool m_LockZAxis = false;
		public:
			float _mass;
			Matrix4 _transform;

			RigidBody();
			RigidBody(Vector3 position, Entity handle);
			RigidBody(float mass, Vector3 position, Quat rotation, Matrix4 transform, Ref<PhysicShape> shape, Entity entity, Vector3 initialVel = Vector3(0, 0, 0));

			void UpdateTransform();

			bool GetLockXAxis() const { return m_LockXAxis; }
			bool GetLockYAxis() const { return m_LockYAxis; }
			bool GetLockZAxis() const { return m_LockZAxis; }

			void setLockXAxis(bool lock) { m_LockXAxis = lock; }
			void setLockYAxis(bool lock) { m_LockYAxis = lock; }
			void setLockZAxis(bool lock) { m_LockZAxis = lock; }

			void SetEntityID(Entity ent);
			Vector3 GetPosition() const { return _position; }
			Quat GetRotation() const { return _rotation; }
			bool HasShape() { return _collisionShape != nullptr; }
			void SetShape(Ref<PhysicShape> shape);
			Ref<PhysicShape> GetShape() const { return _collisionShape; }
			Entity GetEntity() const { return _entity; }
			void AddForce(const Vector3& force);
		};
	}
}
