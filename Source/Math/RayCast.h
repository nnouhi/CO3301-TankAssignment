using namespace std;
#include "CVector3.h"
#include "Entity.h"
#include <memory>
namespace gen
{
	// Singleton design pattern
	class CRayCast
	{
		private:
			CEntity* m_CheckedEntity;
			TFloat32 m_BuildingOutterBounds[6];
			CRayCast();

		public:
			// Stop the compiler generating methods of copy the object
			CRayCast(CRayCast const&) = delete;
			CRayCast& operator=(CRayCast const&) = delete;

			static std::shared_ptr<CRayCast> Instance()
			{
				static std::shared_ptr<CRayCast> s{ new CRayCast };
				return s;
			}
			bool RayBoxIntersect(CVector3 rayStartingPos, CVector3 rayDirection, string objectToCheck);
	};

}

