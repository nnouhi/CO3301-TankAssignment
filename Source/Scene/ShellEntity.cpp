/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
#include "TankEntity.h"
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
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Shell constructor intialises shell-specific data and passes its parameters to the base
// class constructor
CShellEntity::CShellEntity
(
	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	m_LifeDuration = 1.5f;
	m_Radius = 5.0f;
	m_TravelSpeed = 100.0f;
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
// one of the assignment requirements
// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{
	m_LifeDuration -= updateTime;
	if (m_LifeDuration >= 0.0f)
	{
		Matrix().MoveLocalZ(m_TravelSpeed * updateTime);
		
		// Check for collision with any tank
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			CTankEntity* enemyTank = static_cast<CTankEntity*>(entity);
			if (m_Owner != 0)
			{
				CTankEntity* ownerTank = static_cast<CTankEntity*>(m_Owner);

				if (ownerTank != 0 && enemyTank != 0)
				{
					float distance = Distance(entity->Position(), Matrix().Position());
					if (distance <= m_Radius)
					{
						if (ownerTank->GetTeam() != enemyTank->GetTeam() && entity->GetUID() != ownerTank->GetUID())
						{
							SMessage msg;
							msg.from = m_Owner->GetUID();
							msg.type = Msg_Hit;
							msg.damageToApply = ownerTank->GetShellDamage();
							Messenger.SendMessage(enemyTank->GetUID(), msg);
							return false;
						}
					}
					entity = EntityManager.EnumEntity();
				}
			}
			
		}
		EntityManager.EndEnumEntities();
	}
	else
	{
		return false;
	}

	return true; 
}


} // namespace gen
