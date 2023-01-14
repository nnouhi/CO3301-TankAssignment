

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
		Crate Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	class CMineEntity : public CEntity
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		// Shell constructor intialises shell-specific data and passes its parameters to the base
		// class constructor
		CMineEntity
		(
			CEntityTemplate* entityTemplate,
			TEntityUID       UID,
			const TFloat32 respawnTime,
			const TFloat32 damageRadius,
			const string& name = "",
			const CVector3& position = CVector3::kOrigin,
			const CVector3& rotation = CVector3(0.0f, 0.0f, 0.0f),
			const CVector3& scale = CVector3(1.0f, 1.0f, 1.0f)
		);

		// No destructor needed


	/////////////////////////////////////
	//	Public interface
	public:
		virtual bool Update(TFloat32 updateTime);

		const bool IsAlive() { return m_State == Alive; }

	private:
		enum EState
		{
			Alive,
			Respawn,
			Collected
		};

		TFloat32 m_ExplodeTime;
		TFloat32 m_DamageRadius;
		TInt32 m_DamageToApply;
		TFloat32 m_RespawnTime;
		TFloat32 m_MineSpawnHeight;
		TFloat32 m_Gravity;
		CVector3 m_RespawnPosition;
		CVector3 m_AlivePosition;
		CVector3 m_CollectedPosition;
		EState m_State;

		void AliveBehaviour(TFloat32 updateTime);

		void RespawnBehaviour(TFloat32 updateTime);

		void CollectedBehaviour(TFloat32 updateTime);

		void UpdateState(EState newState);

	};


} // namespace gen



