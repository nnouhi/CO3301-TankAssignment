/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "CVector4.h"
#include "RayCast.h"

#define stringify( name ) #name
namespace gen
{

// Some constants that are used
const TFloat32 ShellDistance = 100.0f;
const TFloat32 TurretTurnSpeedMultiplier = 1.5f;
const TFloat32 TankTurnSpeedMultiplier = 3.0f;
const TFloat32 ConeOfVisionWhenPatrolling = 15.0f;
const TFloat32 StopTurretRotationAngle = 1.0f;
const TInt32 AllowedHealthPacksToCollect= 2;


// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
// CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required tank behaviour in the Update function below
extern TEntityUID GetTankUID( int team );

extern CRayCast ray;


/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Tank constructor intialises tank-specific data and passes its parameters to the base
// class constructor
CTankEntity::CTankEntity
(
	CTankTemplate*  tankTemplate,
	TEntityUID      UID,
	TUInt32         team,
	const vector<CVector3> patrolPoints,
	const string&   name /*=""*/,
	const CVector3& position /*= CVector3::kOrigin*/, 
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( tankTemplate, UID, name, position, rotation, scale )
{
	m_TankTemplate = tankTemplate;

	// Tanks are on teams so they know who the enemy is
	m_Team = team;
	m_PatrolPoints = patrolPoints;
	// Initialise other tank data and state
	m_HP = m_TankTemplate->GetMaxHP();
	m_Speed = 0.0f;
	m_State = Inactive;
	m_Timer = 1.0f;
	m_DestructionAnimationTime = 1.0f;
	m_ShellsFired = 0;
	m_ShellCapacity = 10;
	m_CollectedHealthPacks = 0;
	m_ShellsAvailable = m_ShellCapacity;
	m_TargetPoint = CVector3::kOrigin;
	m_ControlledByPlayer = false;
	m_ShouldDestroy = false;
	m_CanAskForAssist = true;
	m_IsCollectingCrate = false;
	m_TankToAssist = 0;
	m_Shell = 0;
	m_ChaseCamera = new CCamera(CVector3(Position().x, Position().y + 3.5f, Position().z));
	m_ChaseCamera->SetNearFarClip(1.0f, 20000.0f);
	m_CurrentPatrolPoint = 0;
	m_TargetRange = 5.0f;
}


// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
// is to be rewritten as one of the assignment requirements
// Return false if the entity is to be destroyed
bool CTankEntity::Update( TFloat32 updateTime )
{
	// Chase camera
	UpdateChaseCamera();

	// Fetch any messages
	SMessage msg;
	while (Messenger.FetchMessage( GetUID(), &msg ))
	{
		// Set state variables based on received messages
		switch (msg.type)
		{
			default:
				break;
			case Msg_Start:
				if (m_State == Inactive)
				{
					UpdateState(Patrol);
				}
				break;
			case Msg_Stop:
				UpdateState(Inactive);
				break;
			case Msg_Hit:
				OnHit(msg.damageToApply);
				break;
			case Msg_Patrol:
					UpdateState(Patrol);
					break;
			case Msg_Evade:
				if (CanEnterEvadeState())
				{
					UpdateState(Evade);
				}
				break;
			case Msg_Help:
				m_TankToAssist = EntityManager.GetEntity(msg.from);
				if (m_TankToAssist)
				{
					SetTargetPoint(m_TankToAssist->Position() + CVector3(Random(-2.5f, 2.5f), 0.0f, Random(-2.5f, 2.5f)));
				}
				UpdateState(Assist);
				break;
		}
	}

	// Tank behaviour
	switch (m_State)
	{
		default: 
			break;
		case Inactive:
			break;
		case Patrol:
			PatrolBehaviour(updateTime);
			break;
		case Aim:
			AimBehaviour(updateTime);
			break;
		case Evade:
			EvadeBehaviour(updateTime);
			break;
		case FindAmmo: case FindHealth:
			FindCrateBehaviour(updateTime, SplitString(GetState(m_State)));
			break;
		case Assist:
			AssistBehaviour(updateTime);
			break;
		case Destruct:
			DestructBehaviour(updateTime, m_ShouldDestroy);
			break;
	}
	
	// Perform movement...
	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ( m_Speed * updateTime );

	// Return false when entity is to be destroyed
	if (m_ShouldDestroy)
	{
		return false;
	}

	return true; // Don't destroy the entity
}

// Tank behaviour methods
void CTankEntity::PatrolBehaviour(TFloat32 updateTime)
{
	SetTargetPoint(m_PatrolPoints[m_CurrentPatrolPoint]);
	RotateTurret(-m_TankTemplate->GetTurretTurnSpeed(), updateTime);

	if (MoveTank(updateTime, m_TankTemplate->GetTurnSpeed()))
	{
		m_CurrentPatrolPoint = (m_PatrolPoints.size() - 1 == m_CurrentPatrolPoint) ? m_CurrentPatrolPoint = 0 : m_CurrentPatrolPoint += 1;
	}

	
	TEntityUID potentialEnemyUID;
	if (CheckTurretAngle(ConeOfVisionWhenPatrolling, potentialEnemyUID))
	{ 
		m_EnemyUID = potentialEnemyUID;
		UpdateState(Aim);
	}
}

void CTankEntity::AimBehaviour(TFloat32 updateTime)
{
	CEntity* enemyTank = EntityManager.GetEntity(m_EnemyUID);
	if (m_Timer >= 0.0f)
	{
		m_Timer -= updateTime;
		if (enemyTank != 0)
		{
			CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
			CVector3 turretRight = Normalise(turretWorldMatrix.XAxis());
			CVector3 turretToOtherTank = Normalise(enemyTank->Position() - turretWorldMatrix.Position());
			bool isRight = Dot(turretRight, turretToOtherTank) > 0.0f ? true : false;
			
			// Stop rotating if turret is almost facing enemy tank
			if (!CheckTurretAngle(StopTurretRotationAngle, enemyTank))
			{
				if (isRight)
				{
					// Target on our right
					RotateTurret(m_TankTemplate->GetTurretTurnSpeed() * TurretTurnSpeedMultiplier, updateTime);
				}
				else 
				{
					// Target on our left 
					RotateTurret(-m_TankTemplate->GetTurretTurnSpeed() * TurretTurnSpeedMultiplier, updateTime);
				}
			}
			else
			{
				turretWorldMatrix.FaceDirection(enemyTank->Position());
			}
		}
	}
	else
	{
		// Don't bother firing a shell if the distance is long
		if (Distance(Position(), enemyTank->Position()) < ShellDistance && !ray.RayBoxIntersect(Position(), GetTurretWorldMatrix().ZAxis(), "Building"))
		{
			if (m_Shell == 0)
			{
				m_Shell = EntityManager.GetTanksShell(this);
			}

			m_Shell->FireShell(enemyTank);
			m_ShellsAvailable--;
			m_ShellsFired++;

			// If tank has sufficient ammo continue to evade state, else refill ammo (if possible)
			bool hasSufficientAmmo = (m_ShellCapacity / 2 >= m_ShellsAvailable) ? false : true;
			if (hasSufficientAmmo)
			{
				UpdateState(Evade);
			}
			else
			{		
				// Move to find ammo state if crates exist, also in case tank has no ammo left enter the state regardless
				if (EntityManager.GetAmmoCrateCount() > 0 || m_ShellsAvailable == 0)
				{
					UpdateState(FindAmmo);
				}
				else
				{
					UpdateState(Evade);
				}
			}
		}
		else
		{
			UpdateState(Patrol);
		}
	}
}

void CTankEntity::EvadeBehaviour(TFloat32 updateTime)
{
	if (MoveTank(updateTime, m_TankTemplate->GetTurnSpeed() * TankTurnSpeedMultiplier))
	{
		if (m_ControlledByPlayer)
		{
			m_ControlledByPlayer = false;
		}
		
		// Reached target point make sure turret is facing the direction the tank is facing
		CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
		CVector3 tankFacing = Normalise(Matrix().ZAxis());

		turretWorldMatrix.FaceDirection(tankFacing);
		UpdateState(Patrol);
	}
	
	RotateTurretToTarget(updateTime);
}

void CTankEntity::FindCrateBehaviour(TFloat32 updateTime, string crateType)
{
	if (MoveTank(updateTime, m_TankTemplate->GetTurnSpeed()))
	{
		if (crateType == "Ammo")
		{
			if (m_ShellsAvailable > 0)
			{
				// Refilled go back to patrolling
				UpdateState(Patrol);
			}
			else
			{
				FindClosestCrate("Ammo");
			}
		}
		else
		{
			UpdateState(Patrol);
		}
	}

	RotateTurretToTarget(updateTime);
}

void CTankEntity::DestructBehaviour(TFloat32 updateTime, bool& shouldDestroy)
{
	if (m_DestructionAnimationTime >= 0)
	{
		m_DestructionAnimationTime -= updateTime;
		// Destruction animation
		Matrix(1).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime * 10.0f);
		Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime * 10.0f);
		Matrix(2).MoveLocalY(50.0f * updateTime);
		shouldDestroy = false;
	}
	else
	{
		shouldDestroy = true;
	}
}

void CTankEntity::AssistBehaviour(TFloat32 updateTime)
{
	if (MoveTank(updateTime, m_TankTemplate->GetTurnSpeed() * TankTurnSpeedMultiplier))
	{
		// Reached target point make sure turret is facing the direction the tank is facing
		CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
		CVector3 tankFacing = Normalise(Matrix().ZAxis());

		turretWorldMatrix.FaceDirection(tankFacing);
		UpdateState(Patrol);
	}

	RotateTurretToTarget(updateTime);
}

// Helper methods
bool CTankEntity::MoveTank(TFloat32 updateTime, TFloat32 rotatingSpeed)
{
	TFloat32 targetDist = Length(m_TargetPoint - Position());
	CVector3 targetVec = m_TargetPoint - Position();
	if (targetDist > m_TargetRange)
	{
		// Turning algorithm, dot products with local X and Z axes (normalise axes in case matrix is scaled)
		TFloat32 forwardDot = Dot(Normalise(targetVec), Normalise(Matrix().ZAxis()));
		TFloat32 rightDot = Dot(Normalise(targetVec), Normalise(Matrix().XAxis()));

		// Turn if not facing right direction
		if (forwardDot < Cos(rotatingSpeed * updateTime))
		{
			if (rightDot > 0.0f)
			{
				Matrix().RotateLocalY(rotatingSpeed * updateTime);
			}
			else
			{
				Matrix().RotateLocalY(-rotatingSpeed * updateTime);
			}
		}
		else
		{
			// Almost facing right direction - set exact facing
			Matrix().FaceTarget(m_TargetPoint);
		}
	}
	/////////////////////////////////////
	//	Move towards target

	// Calculate stopping distance at current speed - using simple stopping distance formula
	TFloat32 StoppingDist = 0.5f * m_Speed * m_Speed / m_TankTemplate->GetAcceleration();

	// If distance > stopping distance then accelerate
	if (targetDist - m_TargetRange > StoppingDist)
	{
		m_Speed += m_TankTemplate->GetAcceleration() * updateTime;
		if (m_Speed > m_TankTemplate->GetMaxSpeed())
		{
			m_Speed = m_TankTemplate->GetMaxSpeed();
		}
	}
	else
	{
		// Otherwise decelerate
		m_Speed -= m_TankTemplate->GetAcceleration() * updateTime;

		// Tank reached the destination decide where to move next
		if (m_Speed < 0)
		{
			m_Speed = 0.0f;
			return true;
		}
	}

	return false;
}

void CTankEntity::RotateTurretToTarget(TFloat32 updateTime)
{
	CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
	CVector3 turretRight = Normalise(turretWorldMatrix.XAxis());
	CVector3 turretFacing = Normalise(turretWorldMatrix.ZAxis());
	CVector3 turretToTarget = Normalise(m_TargetPoint - Position());
	bool isRight = Dot(turretRight, turretToTarget) > 0.0f ? true : false;
	TFloat32 turretRotationOnTarget = ToDegrees(acos(Dot(turretFacing, turretToTarget)));

	if (turretRotationOnTarget > StopTurretRotationAngle)
	{
		if (isRight)
		{
			// Target on our right
			RotateTurret(m_TankTemplate->GetTurretTurnSpeed() * TurretTurnSpeedMultiplier, updateTime);
		}
		else
		{
			// Target on our left 
			RotateTurret(-m_TankTemplate->GetTurretTurnSpeed() * TurretTurnSpeedMultiplier, updateTime);
		}
	}
	else
	{
		turretWorldMatrix.FaceDirection(m_TargetPoint);
	}
}

void CTankEntity::RotateTurret(TFloat32 amount, TFloat32 updateTime)
{
	Matrix(2).RotateLocalY(amount * updateTime);
}

bool CTankEntity::CheckTurretAngle(TFloat32 degreesBeforeAim, TEntityUID &enemyUID)
{
	CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
	vector<CTankEntity*> enemyTanks = EntityManager.GetEnemyTeamTanks(GetTeam());
	for each (CTankEntity * enemyTank in enemyTanks)
	{
		// Don't bother aiming if the distance is long
		TFloat32 distance = Distance(Position(), enemyTank->Position());
		if (distance < ShellDistance && !ray.RayBoxIntersect(Position(), GetTurretWorldMatrix().ZAxis(), "Building"))
		{
			// Find angle between 2 vectors: cosθ= u⋅v / ||u|| ||v||
			CVector3 turretFacing = Normalise(turretWorldMatrix.ZAxis());
			CVector3 turretToOtherTank = Normalise(enemyTank->Position() - Position());
			TFloat32 rotationOnTarget = Dot(turretFacing, turretToOtherTank);
			TFloat32 rotationOnTargetDeg = ToDegrees(acos(rotationOnTarget));
			if (rotationOnTargetDeg < degreesBeforeAim)
			{
				enemyUID = enemyTank->GetUID();
				return true;
			}
			else
			{
				enemyUID = -1;
			}
		}
	}
			
	enemyUID = -1;
	return false;
}

bool CTankEntity::CheckTurretAngle(TFloat32 degreesBeforeAim, CEntity* enemyTank)
{
	CMatrix4x4 turretWorldMatrix = GetTurretWorldMatrix();
	
	// Find angle between 2 vectors: cosθ= u⋅v / ||u|| ||v||
	CVector3 turretFacing = Normalise(turretWorldMatrix.ZAxis());
	CVector3 turretToOtherTank = Normalise(enemyTank->Position() - Position());
	TFloat32 rotationOnTarget = Dot(turretFacing, turretToOtherTank);
	TFloat32 rotationOnTargetDeg = ToDegrees(acos(rotationOnTarget));
				
	if (rotationOnTargetDeg <= degreesBeforeAim)
	{
		return true;
	}	
	
	return false;
}

void CTankEntity::FindClosestCrate(string crateType)
{
	TFloat32 closestDistanceCrate = D3D10_FLOAT32_MAX;
	CCRateEntity* closestCrateEntity = 0;
	vector<CCRateEntity*> crateEntities = EntityManager.GetCrateEntities(crateType);
	for each (CCRateEntity* crateEntity in crateEntities)
	{
		TFloat32 distance = Distance(crateEntity->Position(), Position());
		if (distance < closestDistanceCrate)
		{
			closestCrateEntity = crateEntity;
			closestDistanceCrate = distance;
		}	
	}

	if (closestCrateEntity != 0)
	{
		closestCrateEntity->SetTargeted(true);
		m_TargetPoint = closestCrateEntity->Position();
	}
	else 
	{
		if (crateType == "Ammo")
		{
			if (m_ShellsAvailable > 0)
			{
				// Didn't find any ammo crates but has ammo to carry on 
				UpdateState(Evade);
			}
			else
			{
				m_TargetPoint = GetRandomPoint(60.0f, 0.0f, 60.0f);
			}
		}
	}
}

void CTankEntity::UpdateChaseCamera()
{
	CVector3 facingVector = Normalise(Matrix().ZAxis());
	CVector3 upVector = Normalise(Matrix().YAxis());

	// Multiplication to get the desired result
	m_ChaseCamera->Position() = Position() - facingVector * 20.0f + upVector * 5.0f;
	m_ChaseCamera->Matrix().FaceTarget(Position());
}

bool CTankEntity::IsAliveAfterHit(TInt32 damageToApply)
{
	return m_HP - damageToApply > 0;
}

void CTankEntity::UpdateState(EState newState)
{

	// Validate 
	if (newState == m_State)
	{
		return;
	}

	m_Speed = 0.0f;

	// Prepare new variables (if any) that will be used in new state
	switch (newState)
	{
		default:
			break;
		case Inactive:
			break;
		case Patrol:
			m_Timer = 1.0f;
			break;
		case Aim:
			break;
		case Evade:
			if (!m_ControlledByPlayer)
			{
				SetTargetPoint(CVector3(Position() + GetRandomPoint(40.0f, 0.0f, 40.0f)));
			}
			break;
		case FindAmmo: case FindHealth:
			// NOTE: SplitString is a method that I am using to Split FindAmmo and FindHealth states to Ammo and Health
			FindClosestCrate(SplitString(GetState(newState)));
			break;
		case Assist:
			break;
		case Destruct:
			break;
	}

	m_State = newState;
}

void CTankEntity::OnHit(TInt32 damageToApply)
{
	if (IsAliveAfterHit(damageToApply))
	{
		m_HP -= damageToApply;

		// If at 40% health call for assistance (can call for assistance once) and search for any health crates
		if (m_HP <= m_TankTemplate->GetMaxHP() * 0.4f)
		{
			if (m_CanAskForAssist)
			{
				// Send a 'help' message to the nearest teammate
				vector<CTankEntity*> friendlyTanks = EntityManager.GetTeamTanks(GetTeam(), this);
				CTankEntity* assistingTank = 0;
				TFloat32 distance = 0.0f;
				TFloat32 nearestDistance = D3D10_FLOAT32_MAX;
				for each (CTankEntity * tank in friendlyTanks)
				{
					distance = Distance(Position(), tank->Position());
					if (distance < nearestDistance)
					{
						nearestDistance = distance;
						assistingTank = tank;
					}
				}

				if (assistingTank != 0)
				{
					m_CanAskForAssist = false;

					SMessage msg;
					msg.from = GetUID();
					msg.type = Msg_Help;
					Messenger.SendMessage(assistingTank->GetUID(), msg);
				}
			}

			// Maybe changing to health state when < 40% is a bit broken?
			if (EntityManager.GetHealthCrateCount() > 0 && !m_IsCollectingCrate && m_CollectedHealthPacks < AllowedHealthPacksToCollect)
			{
				SetIsCollectingCrate(true);
				UpdateState(FindHealth);
			}
		}

	}
	else
	{
		m_HP = 0;
		UpdateState(Destruct);
	}
}

} // namespace gen
