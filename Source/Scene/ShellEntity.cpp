/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
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

const TFloat32 BarrelLength = 4.0f;


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
	CTankEntity* owner,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	m_LifeDuration = 1.5f;
	m_Radius = 5.0f;
	m_TravelSpeed = 100.0f;
	m_Owner = owner;
	m_State = Destroyed;
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
// one of the assignment requirements
// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{
	switch (m_State)
	{
		default:
			break;
		case Alive:
			AliveBehaviour(updateTime);
			break;
		case Destroyed:
			DestroyedBehaviour(updateTime);
			break;
	}

	return true; 
}

void CShellEntity::FireShell(CEntity* entityToFireTo)
{
	CMatrix4x4 turretWorldMatrix = m_Owner->GetTurretWorldMatrix();
	CVector3 turretRotation;
	turretWorldMatrix.DecomposeAffineEuler(NULL, &turretRotation, NULL);
	SetPosition(turretWorldMatrix.Position() + turretWorldMatrix.ZAxis() * BarrelLength);
	SetRotation(entityToFireTo->Position());
	UpdateState(Alive);
}

void CShellEntity::AliveBehaviour(TFloat32 updateTime)
{
	m_LifeDuration -= updateTime;
	if (m_LifeDuration >= 0.0f)
	{
		Matrix().MoveLocalZ(m_TravelSpeed * updateTime);
		
		// Check for collision with any tank (excluding owning tank)
		vector<CTankEntity*> tanks = EntityManager.GetTankEntities(m_Owner);
		for each (CTankEntity * tank in tanks)
		{
			TFloat32 distance = Distance(tank->Position(), Matrix().Position());
			if (distance <= m_Radius)
			{
				if (m_Owner->GetTeam() != tank->GetTeam())
				{
					SMessage msg;
					msg.from = m_Owner->GetUID();
					msg.type = Msg_Hit;
					msg.damageToApply = m_Owner->GetShellDamage();
					Messenger.SendMessage(tank->GetUID(), msg);
				}
				UpdateState(Destroyed);
			}
		}
	}
	else
	{
		UpdateState(Destroyed);
	}
}

void CShellEntity::DestroyedBehaviour(TFloat32 updateTime)
{
}

void CShellEntity::UpdateState(EState newState)
{
	switch (newState)
	{
		default:
			break;
		case Alive:
			{
			}	
			break;
		case Destroyed:
			{
				m_LifeDuration = 1.5f;
				SetPosition(CVector3(0.0f, -10.0f, 0.0f));
			}
			break;
	}

	m_State = newState;
}


} // namespace gen
