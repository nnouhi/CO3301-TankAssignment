#include "MineEntity.h"
#include "EntityManager.h"
#include "Messenger.h"



namespace gen
{
	//	Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
	//	Can then access other entity's data. See the CEntityManager.h file for functions. Example:
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	CMineEntity::CMineEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const TFloat32 respawnTime,
		const TFloat32 damageRadius,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		m_RespawnTime = respawnTime;
		m_DamageRadius = damageRadius;
		// Could also assign the below 2 variables through xml with a randomizer, just wanted to do this like this
		m_ExplodeTime = Random(2.0f, 6.0f);
		m_DamageToApply = Random(25, 100);
		m_Gravity = 20.0f;
		m_MineSpawnHeight = 30.0f;
		m_CollectedPosition = CVector3::kOrigin;
		m_RespawnPosition = CVector3::kOrigin;
		m_AlivePosition = CVector3::kOrigin;
		m_State = Collected;
	}

	bool CMineEntity::Update(TFloat32 updateTime)
	{
		switch (m_State)
		{
		default:
			break;
		case Alive:
			AliveBehaviour(updateTime);
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

	void CMineEntity::AliveBehaviour(TFloat32 updateTime)
	{
		if (m_ExplodeTime > 0)
		{
			m_ExplodeTime -= updateTime;
		}
		else
		{
			vector<CTankEntity*> tanksToDamage;
			// Find out if any of the tanks is able to pick up this crate
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != 0)
			{
				if (Distance(Position(), entity->Position()) < m_DamageRadius)
				{
					CTankEntity* tankEntity = static_cast<CTankEntity*>(entity);
					if (tankEntity != 0)
					{
						tanksToDamage.push_back(tankEntity); 
					}
				}
				entity = EntityManager.EnumEntity();
			}
			EntityManager.EndEnumEntities();
			
			if (tanksToDamage.size() > 0)
			{
				SMessage msg;
				msg.from = GetUID();
				msg.type = Msg_Hit;
				msg.damageToApply = m_DamageToApply;
				
				for each (CTankEntity* tank in tanksToDamage)
				{
					Messenger.SendMessage(tank->GetUID(), msg);
				}
			}

			m_ExplodeTime = Random(2.0f, 6.0f);//
			UpdateState(Collected);
		}
	}
	

	void CMineEntity::RespawnBehaviour(TFloat32 updateTime)
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

	void CMineEntity::CollectedBehaviour(TFloat32 updateTime)
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

	void CMineEntity::UpdateState(EState newState)
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
				m_RespawnPosition = CVector3(Random(-100.0f, 100.0f), m_MineSpawnHeight, Random(-50.0f, 50.0f));
				m_AlivePosition = m_RespawnPosition - CVector3(0.0f, m_MineSpawnHeight - 1.5f, 0.0f);
				Matrix().SetPosition(m_RespawnPosition);
				break;
		}

		m_State = newState;
	}
} // namespace gen
