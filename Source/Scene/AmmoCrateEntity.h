

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "CrateEntity.h"
namespace gen
{

	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Ammo Crate Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	class CAmmoCrateEntity : public CCRateEntity
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		CAmmoCrateEntity
		(
			CEntityTemplate* entityTemplate,
			TEntityUID       UID,
			const TFloat32 rotationSpeed = 5.0f,
			const TFloat32 respawnTime = 5.0f,
			const TFloat32 pickUpDistance = 5.0f,
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

		/////////////////////////////////////
		//	Private interface
	private:
		TInt32 m_AmountOfShells;
		
		void AliveBehaviour(TFloat32 updateTime) override;

		//void RespawnBehaviour(TFloat32 updateTime) override;

		//void CollectedBehaviour(TFloat32 updateTime) override;

		//void UpdateState(EState newState) override;
	};


} // namespace gen



