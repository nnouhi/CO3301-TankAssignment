#include "CrateEntity.h"

namespace gen
{
	CCRateEntity::CCRateEntity
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
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		m_RotationSpeed = rotationSpeed;
		m_RespawnTime = respawnTime;
		m_PickUpDistance = pickUpDistance;
		m_Gravity = 20.0f;
		m_CrateSpawnHeight = 30.0f;
		m_CollectedPosition = CVector3::kOrigin;
		m_RespawnPosition = CVector3::kOrigin;
		m_AlivePosition = CVector3::kOrigin;
		m_State = Collected;
		m_IsTargeted = false;
	}

	bool CCRateEntity::Update(TFloat32 updateTime)
	{
		switch (m_State)
		{
			default:
				break;
			case Alive:
				AliveBehaviour(updateTime);
				if (m_IsTargeted)
				{
					m_RetargetCooldown -= updateTime;
					// In case the crate was not claimed by the tank that was supposed to, reset it back to not targeted
					// in order for other tanks to be able to colelct it
					if (m_RetargetCooldown <= 0.0f)
					{
						SetTargeted(false);
						m_RetargetCooldown = 15.0f;
					}
				}
				break;
			case Collected:
				CollectedBehaviour(updateTime);
				break;
			case Respawn:
				RespawnBehaviour(updateTime);
				break;
			}
		
		return true;
	}

	void CCRateEntity::AliveBehaviour(TFloat32 updateTime)
	{
		// Implementated in derived classes because here goes specific functionality 
	}

	void CCRateEntity::RespawnBehaviour(TFloat32 updateTime)
	{
		if (Position().y > m_AlivePosition.y)
		{
			Position().y -= updateTime * m_Gravity;
		}
		else
		{
			UpdateState(Alive);
		}
	}

	void CCRateEntity::CollectedBehaviour(TFloat32 updateTime)
	{
		if (m_RespawnTime > 0.0f)
		{
			m_RespawnTime -= updateTime;
		}
		else
		{
			UpdateState(Respawn);
		}
	}

	void CCRateEntity::UpdateState(EState newState)
	{
		switch (newState)
		{
			default:
				break;
			case Alive:
				break;
			case Collected:
				// In this state the crate is hidden bellow the floor
				m_CollectedPosition = CVector3(0.0f, -10.0f, 0.0f);
				Matrix().SetPosition(m_CollectedPosition);
				break;
			case Respawn:
				// Update variables to be used in respawn state
				m_RespawnTime = Min(5.0f, Random(10.0f, m_RespawnTime));
				m_RespawnPosition = CVector3(Random(-100.0f , 100.0f), m_CrateSpawnHeight, Random(-50.0f, 50.0f));
				m_AlivePosition = m_RespawnPosition - CVector3(0.0f, m_CrateSpawnHeight, 0.0f);
				Matrix().SetPosition(m_RespawnPosition);
				break;
		}

		m_State = newState;
	}



} // namespace gen
