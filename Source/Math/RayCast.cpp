#include "RayCast.h"
#include "EntityManager.h"
#define MIN_X 0
#define MIN_Y 1
#define MIN_Z 2
#define MAX_X 3
#define MAX_Y 4
#define MAX_Z 5

namespace gen
{
	extern CEntityManager EntityManager;

	CRayCast::CRayCast()
	{
		// Not using Template()->Mesh()->MinBounds()/MaxBounds() because it returns invalid values for Z axis
		// -bounds
		m_BuildingOutterBounds[MIN_X] = -7.36113f; // X
		m_BuildingOutterBounds[MIN_Y] = -0.148627f; // Y
		m_BuildingOutterBounds[MIN_Z] = -4.34613f; // Z
		// +bounds
		m_BuildingOutterBounds[MAX_X] = 5.11745f;
		m_BuildingOutterBounds[MAX_Y] = 11.1836f;
		m_BuildingOutterBounds[MAX_Z] = 5.35663f;
	}

	bool CRayCast::RayBoxIntersect(CVector3 rayStartingPos, CVector3 rayDirection, string objectToCheck)
	{
		// Adaptation of the code found at: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
		vector<CEntity*> entities = EntityManager.GetEntities(objectToCheck);
		vector<bool> collidedWith;

		for each (CEntity * entity in entities)
		{
			CVector3 checkedEntityPos = entity->Position();
			rayDirection.Normalise();

			TFloat32 xMin = checkedEntityPos.x + m_BuildingOutterBounds[MIN_X];
			TFloat32 xMax = checkedEntityPos.x + m_BuildingOutterBounds[MAX_X];
			TFloat32 yMin = checkedEntityPos.y + m_BuildingOutterBounds[MIN_Y];
			TFloat32 yMax = checkedEntityPos.y + m_BuildingOutterBounds[MAX_Y];
			TFloat32 zMin = checkedEntityPos.z + m_BuildingOutterBounds[MIN_Z];
			TFloat32 zMax = checkedEntityPos.z + m_BuildingOutterBounds[MAX_Z];

			CVector3 invRayDirection;
			invRayDirection.x = 1.0f / rayDirection.x;
			invRayDirection.y = 1.0f / rayDirection.y;
			invRayDirection.z = 1.0f / rayDirection.z;

			TFloat32 t1 = (xMin - rayStartingPos.x) * invRayDirection.x;
			TFloat32 t2 = (xMax - rayStartingPos.x) * invRayDirection.x;
			TFloat32 t3 = (yMin - rayStartingPos.y) * invRayDirection.y;
			TFloat32 t4 = (yMax - rayStartingPos.y) * invRayDirection.y;
			TFloat32 t5 = (zMin - rayStartingPos.z) * invRayDirection.z;
			TFloat32 t6 = (zMax - rayStartingPos.z) * invRayDirection.z;

			TFloat32 tmin = Max(Max(Min(t1, t2), Min(t3, t4)), Min(t5, t6));
			TFloat32 tmax = Min(Min(Max(t1, t2), Max(t3, t4)), Max(t5, t6));

			// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
			// or
			// if tmin > tmax, ray doesn't intersect AABB
			if (tmax < 0.0f || tmin > tmax)
			{
				collidedWith.push_back(false);
			}
			else
			{
				collidedWith.push_back(true);
			}
		}

		// If the ray collided with one of the buildings
		for each (bool collided in collidedWith)
		{
			if (collided)
			{
				return true;
			}
		}

		// Didn't collide with any of the buildings
		return false;
	}
}
