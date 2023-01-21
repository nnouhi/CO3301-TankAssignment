/*******************************************
	TankEntity.h

	Tank entity template and entity classes
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"


namespace gen
{

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Template Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank template inherits the type, name and mesh from the base template and adds further
// tank specifications
class CTankTemplate : public CEntityTemplate
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank entity template constructor sets up the tank specifications - speed, acceleration and
	// turn speed and passes the other parameters to construct the base class
	CTankTemplate
	(
		const string& type, const string& name, const string& meshFilename,
		TFloat32 maxSpeed, TFloat32 acceleration, TFloat32 turnSpeed,
		TFloat32 turretTurnSpeed, TUInt32 maxHP, TUInt32 shellDamage
	) : CEntityTemplate( type, name, meshFilename )
	{
		// Set tank template values
		m_MaxSpeed = maxSpeed;
		m_Acceleration = acceleration;
		m_TurnSpeed = turnSpeed;
		m_TurretTurnSpeed = turretTurnSpeed;
		m_MaxHP = maxHP;
		m_ShellDamage = shellDamage;
	}

	// No destructor needed (base class one will do)


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	//	Getters

	const TFloat32 GetMaxSpeed() { return m_MaxSpeed; }

	const TFloat32 GetAcceleration() { return m_Acceleration; }

	const TFloat32 GetTurnSpeed() { return m_TurnSpeed; }

	const TFloat32 GetTurretTurnSpeed() { return m_TurretTurnSpeed; }

	const TInt32 GetMaxHP() { return m_MaxHP; }

	const TInt32 GetShellDamage() { return m_ShellDamage; }


/////////////////////////////////////
//	Private interface
private:

	// Common statistics for this tank type (template)
	TFloat32 m_MaxSpeed;        // Maximum speed for this kind of tank
	TFloat32 m_Acceleration;    // Acceleration  -"-
	TFloat32 m_TurnSpeed;       // Turn speed    -"-
	TFloat32 m_TurretTurnSpeed; // Turret turn speed    -"-

	TUInt32  m_MaxHP;           // Maximum (initial) HP for this kind of tank
	TUInt32  m_ShellDamage;     // HP damage caused by shells from this kind of tank
};



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance and state data. It overrides the update function to perform the tank
// entity behaviour
// The shell code performs very limited behaviour to be rewritten as one of the assignment
// requirements. You may wish to alter other parts of the class to suit your game additions
// E.g extra member variables, constructor parameters, getters etc.
class CShellEntity; // Had some linking issues with header files
class CTankEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity
	(
		CTankTemplate* tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		const vector<CVector3> patrolPoints,
		const string&   name = "",
		const CVector3& position = CVector3::kOrigin, 
		const CVector3& rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// No destructor needed


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Getters

	const TFloat32 GetSpeed() { return m_Speed; }

	const string GetState() 
	{
		switch (m_State)
		{
			case Inactive: return "Inactive"; break;
			case Patrol: return "Patrol"; break;
			case Aim: return "Aim"; break;
			case Evade: return "Evade"; break;
			case FindAmmo: return "Find Ammo"; break;
			case FindHealth: return "Find Health"; break;
			case Assist : return "Assist"; break;
			case Destruct: return "Destruct"; break;
		}

		return "Unknown";
	}

	const CMatrix4x4 GetTurretWorldMatrix() { return Matrix(2) * Matrix(); }

	const TInt32 GetTeam() { return m_Team; }

	const TInt32 GetHP() { return  m_HP; }

	const TInt32 GetMaxHP() { return m_TankTemplate->GetMaxHP(); }

	const TInt32 GetShellsFired() { return m_ShellsFired; }

	const TInt32 GetShellsAvailable() { return m_ShellsAvailable; }

	const TInt32 GetShellCapacity() { return m_ShellCapacity; }

	const bool CanEnterEvadeState() { return m_State != Inactive && m_State != Destruct; }

	const bool GetAliveStatus() { return m_State != Destruct; }

	CCamera* GetChaseCamera() { return m_ChaseCamera; }

	const TFloat32 GetDestructionTime() { return m_DestructionAnimationTime; }

	const TInt32 GetShellDamage() { return m_TankTemplate->GetShellDamage(); }

	const CVector3 GetTargetPosition() { return m_TargetPoint; }

	const CVector3 GetRandomPoint(TFloat32 randomX, TFloat32 randomY, TFloat32 randomZ) { return CVector3(Random(-randomX, randomX), Random(-randomY, randomY), Random(-randomZ, randomZ)); }

	virtual bool Update( TFloat32 updateTime );


	/////////////////////////////////////
	// Setters

	void SetIsCollectingCrate(bool isCollectingCrate) { m_IsCollectingCrate = isCollectingCrate; }

	void IncrementCollectedHealthPacks() { m_CollectedHealthPacks++; }

	void SetTargetPoint(CVector3 newTargetPoint, bool controlledByPlayer = false) { m_ControlledByPlayer = controlledByPlayer; m_TargetPoint = newTargetPoint; }

	void SetShells(TInt32 amountOfShells) { m_ShellsAvailable = amountOfShells; }

	void RestoreShells(TInt32 amountOfShellsToGive) { m_ShellsAvailable += amountOfShellsToGive; }
	
	void RestoreHealth(TInt32 amountOfHealthToGive) { m_HP += amountOfHealthToGive; }

	void SetPatrolPoints(vector<CVector3> patrolPoints) { m_PatrolPoints = patrolPoints; }

/////////////////////////////////////
//	Private interface
private:

	/////////////////////////////////////
	// Types

	// States available for a tank - placeholders for shell code
	enum EState
	{
		Inactive,
		Patrol,
		Aim,
		Evade,
		FindAmmo,
		FindHealth,
		Assist,
		Destruct
	};

	/////////////////////////////////////
	// Data

	// The template holding common data for all tank entities
	CTankTemplate* m_TankTemplate;

	// Tank data
	TUInt32  m_Team;  
	TInt32 m_CurrentPatrolPoint;
	TInt32   m_HP;    
	TInt32 m_ShellCapacity;
	TInt32 m_ShellsAvailable;
	TInt32 m_ShellsFired;
	TInt32 m_ShellDamage;
	TInt32 m_CollectedHealthPacks;
	TFloat32 m_DestructionAnimationTime;
	TFloat32 m_Speed; 
	TFloat32 m_TargetRange;
	TFloat32 m_Timer;   
	vector<CVector3> m_PatrolPoints;
	CVector3 m_TargetPoint;
	EState   m_State; 
	TEntityUID m_EnemyUID;
	CCamera* m_ChaseCamera;
	CEntity* m_TankToAssist;
	CShellEntity* m_Shell;
	bool m_ControlledByPlayer;
	bool m_ShouldDestroy;
	bool m_CanAskForAssist;
	bool m_IsCollectingCrate;
	// State behaviour methods
	void PatrolBehaviour(TFloat32 updateTime);

	void EvadeBehaviour(TFloat32 updateTime);

	void AimBehaviour(TFloat32 updateTime);

	void FindCrateBehaviour(TFloat32 updateTime, string crateType);

	void DestructBehaviour(TFloat32 updateTime, bool &shouldDestroy);

	void AssistBehaviour(TFloat32 updateTime);

	// State behaviour helper method
	bool MoveTank(TFloat32 updateTime, TFloat32 rotatingSpeed);

	void RotateTurretToTarget(TFloat32 updateTime);

	void RotateTurret(TFloat32 amount, TFloat32 updateTime);

	bool CheckTurretAngle(TFloat32 degreesBeforeAim, TEntityUID &enemyUID);

	bool CheckTurretAngle(TFloat32 degreesBeforeAim, CEntity* enemyTank);

	void UpdateState(EState newState);

	void FindClosestCrate(string crateType);

	void UpdateChaseCamera();

	bool IsAliveAfterHit(TInt32 damageToApply);

	void OnHit(TInt32 damageToApply);

	const string GetState(EState state)
	{
		switch (state)
		{
			case Inactive: return "Inactive"; break;
			case Patrol: return "Patrol"; break;
			case Aim: return "Aim"; break;
			case Evade: return "Evade"; break;
			case FindAmmo: return "FindAmmo"; break;
			case FindHealth: return "FindHealth"; break;
			case Assist: return "Assist"; break;
			case Destruct: return "Destruct"; break;
		}

		return "Unknown";
	}
};


} // namespace gen
