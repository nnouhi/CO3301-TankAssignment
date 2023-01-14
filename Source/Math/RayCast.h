using namespace std;
#include "EntityManager.h"
#include "CVector3.h"
namespace gen
{

class CRayCast
{
	private:
		CEntityManager* m_EntityManager;
		CEntity* m_CheckedEntity;
		TFloat32 m_BuildingOutterBounds[6];
	public:
		CRayCast(CEntityManager* entityManager);

		bool RayBoxIntersect(CVector3 rayStartingPos, CVector3 rayDirection, string objectToCheck);
};

}

