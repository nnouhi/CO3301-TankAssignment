
#include "HealthCrateEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

	// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
	// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
	//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
	// Will be needed to implement the required shell behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);


	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CHealthCrateEntity::CHealthCrateEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const TFloat32 rotationSpeed,
		const TFloat32 respawnTime,
		const TFloat32 pickUpDistance,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CCRateEntity(entityTemplate, UID, rotationSpeed, respawnTime, pickUpDistance, name, position, rotation, scale)
	{
		m_AmountOfHealthToRestore = Random(50, 100);
	}

	bool CHealthCrateEntity::Update(TFloat32 updateTime)
	{
		CCRateEntity::Update(updateTime);
		return true;
	}

	void CHealthCrateEntity::AliveBehaviour(TFloat32 updateTime)
	{
		Matrix().RotateLocalY(m_RotationSpeed * updateTime);

		// Find out if any of the tanks is able to pick up this crate
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			if (Distance(Position(), entity->Position()) < m_PickUpDistance)
			{
				CTankEntity* tankEntity = static_cast<CTankEntity*>(entity);
				TInt32 currentTankHPs = tankEntity->GetHP();
				TInt32 tankMaxHP = tankEntity->GetMaxHP();
				TInt32 healthToGive = Min(tankMaxHP - currentTankHPs, m_AmountOfHealthToRestore);

				tankEntity->RestoreHealth(healthToGive);
				tankEntity->SetIsCollectingCrate(false);
				tankEntity->IncrementCollectedHealthPacks();
				UpdateState(Collected);
			}

			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}


} // namespace gen
