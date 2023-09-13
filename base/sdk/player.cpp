#include "entity.h"
#include "../context.h"
#include "../features/rage/exploits.h"

bool CBasePlayer::IsTeammate( CBasePlayer* player ) {
	if ( !player )
		player = ctx.m_pLocal;

	if ( !player )
		return false;

	if ( Displacement::Cvars.mp_teammates_are_enemies != nullptr
		&& Displacement::Cvars.mp_teammates_are_enemies->GetBool( ) && this != ctx.m_pLocal ) // you see that guy over there? yeah. he's not ur friend
		return false;

	return ( this->m_iTeamNum( ) == player->m_iTeamNum( ) );
}

int CBasePlayer::GetSequenceActivity( int sequence ) {
	studiohdr_t* pStudioHdr = Interfaces::ModelInfo->GetStudioModel( this->GetModel( ) );
	if ( pStudioHdr == nullptr )
		return -1;

	using GetSequenceActivityFn = int( __fastcall* )( void*, void*, int );
	static auto oGetSequenceActivity = ( GetSequenceActivityFn )Displacement::Sigs.GetSequenceActivity;
	return oGetSequenceActivity( this, pStudioHdr, sequence );
}

bool CBasePlayer::IsDead( ) {
	if ( m_iHealth( ) <= 0 )
		return true;

	return m_lifeState( );
}

CWeaponCSBase* CBasePlayer::GetWeapon( ) {
	return static_cast< CWeaponCSBase * >( Interfaces::ClientEntityList->GetClientEntityFromHandle( this->m_hActiveWeapon( ) ) );
}

Vector CBasePlayer::GetEyePosition( float yaw, float pitch ) {
	auto eyePos{ this->GetAbsOrigin( ) + this->m_vecViewOffset( ) };// should this just be uninterpolated origin?

	const auto state{ this->m_pAnimState( ) };
	if ( !state )
		return eyePos;

	if ( this == ctx.m_pLocal && state->m_pPlayer && ( state->m_bLanding || this->m_flDuckAmount( ) || this->m_hGroundEntity( ) == -1 ) ) {
		const auto backupState{ *state };
		CAnimationLayer backupLayers[ 13 ]{ };
		std::memcpy( backupLayers, this->m_AnimationLayers( ), 13 * sizeof CAnimationLayer );
		const auto backupPoseParam{ this->m_flPoseParameter( ) };

		const auto j{ ( Interfaces::ClientState->iLastOutgoingCommand + 1 ) % 150 };

		auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
		auto& curLocalData{ ctx.m_cLocalData.at( j ) };

		if ( curLocalData.m_flSpawnTime == this->m_flSpawnTime( )
			&& curUserCmd.iTickCount != INT_MAX ) {
			const auto backupTickbase{ this->m_nTickBase( ) };
			const auto backupFlags{ this->m_fFlags( ) };
			const auto backupVel{ this->m_vecAbsVelocity( ) };
			const auto backupDuckAmount{ this->m_flDuckAmount( ) };

			const auto backupJumping{ Features::AnimSys.m_bJumping };
			const auto backupLowerBodyRealignTimer{ Features::AnimSys.m_flLowerBodyRealignTimer };

			this->m_nTickBase( ) = curLocalData.PredictedNetvars.m_nTickBase;
			this->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
			this->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
			this->m_flDuckAmount( ) = curLocalData.PredictedNetvars.m_flDuckAmount;

			Features::AnimSys.UpdateLocal( { pitch, yaw, 0.f }, true, curUserCmd );

			this->m_nTickBase( ) = backupTickbase;
			this->m_fFlags( ) = backupFlags;
			this->m_vecAbsVelocity( ) = backupVel;
			this->m_flDuckAmount( ) = backupDuckAmount;

			Features::AnimSys.m_bJumping = backupJumping;
			Features::AnimSys.m_flLowerBodyRealignTimer = backupLowerBodyRealignTimer;
		}

		this->m_flPoseParameter( ).at( 12u ) = ( std::clamp( std::remainder(
			pitch, 360.f
		), -90.f, 90.f ) + 90.f ) / 180.f;


		const auto matrix{ new matrix3x4a_t[ 256 ] };

		//this->SetAbsOrigin( this->m_vecOrigin( ) );

		Features::AnimSys.SetupBonesRebuilt( this, matrix, BONE_USED_BY_HITBOX, ctx.m_flFixedCurtime, true );

		this->m_flPoseParameter( ) = backupPoseParam;
		*state = backupState;
		std::memcpy( this->m_AnimationLayers( ), backupLayers, 13 * sizeof CAnimationLayer );

		//static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Displacement::Sigs.LookupBone ) };
		//const auto boneIndex{ lookupBone( v12->pEntity, _( "head_0" ) ) };

		const auto headPosZ{ matrix[ 8 ].GetOrigin( ).z + 1.7f };

		if ( eyePos.z > headPosZ ) {
			const auto v8{ std::max( std::min( ( std::abs( eyePos.z - headPosZ ) - 4.f ) / 6.f, 1.f ), 0.f ) };

			eyePos.z += ( headPosZ - eyePos.z )
				* ( ( ( v8 * v8 ) * 3.f )
					- ( ( ( v8 * v8 ) * 2.f ) * v8 ) );
		}

		delete[ ] matrix;
	}

	return eyePos;
}

bool CBasePlayer::IsHostage( ) {
	auto client = this->GetClientClass( );
	return client->nClassID == EClassIndex::CHostage;
}

float CBasePlayer::m_flMaxSpeed( ) {
	const auto weapon{ this->GetWeapon( ) };
	if ( !weapon )
		return 260.f;

	const auto weaponData{ weapon->GetCSWeaponData( ) };
	if ( !weaponData )
		return 260.f;

	return this->m_bIsScoped( ) ? weaponData->flMaxSpeedAlt : weaponData->flMaxSpeed;
}

bool CBasePlayer::CanShoot( bool secondary ) {
	ctx.m_bRevolverCanCock = false;

	if ( this->m_fFlags( ) & 0x40 )
		return false;

	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) )
		return false;

	const auto weapon{ this->GetWeapon( ) };
	if ( !weapon )
		return false;

	if ( this->m_bWaitForNoAttack( ) )
		return false;

	if ( this->m_iPlayerState( ) > 0 )
		return false;

	if ( this->m_bIsDefusing( ) )
		return false;	
	
	//if ( weapon->m_bReloading( ) )
	//	return false;

	const auto& idx{ weapon->m_iItemDefinitionIndex( ) };

	//if ( ctx.m_flLastPrimaryAttack == weapon->m_flNextPrimaryAttack( ) )
	//	return false;

	// ghetto af
	//if ( ctx.m_flNewPacketTime <= ctx.m_iLastStopTime )
	//	return false;

	if ( ctx.m_flFixedCurtime < this->m_flNextAttack( ) )
		return false;

	if ( ( idx == WEAPON_GLOCK || idx == WEAPON_FAMAS )
		&& weapon->m_iBurstShotsRemaining( ) > 0 ) {
		if ( ctx.m_flFixedCurtime >= weapon->m_fNextBurstShot( ) )
			return true;
	}

	if ( idx == WEAPON_C4 )
		return true;

	const auto weaponData{ weapon->GetCSWeaponData( ) };
	if ( !weaponData )
		return false;

	if ( weaponData->nWeaponType >= WEAPONTYPE_PISTOL && weaponData->nWeaponType <= WEAPONTYPE_MACHINEGUN && weapon->m_iClip1( ) < 1 )
		return false;

	if ( idx != WEAPON_REVOLVER ) {
		if ( ( !secondary && ctx.m_flFixedCurtime < weapon->m_flNextPrimaryAttack( ) )
			|| ( secondary && ctx.m_flFixedCurtime < weapon->m_flNextSecondaryAttack( ) ) )
			return false;

		return true;
	}

	ctx.m_bRevolverCanCock = true;

	if ( weapon->m_nSequence( ) != 5 )
		return false;

	return weapon->m_flPostponeFireReadyTime( ) != FLT_MAX 
		&& weapon->m_flPostponeFireReadyTime( ) < ctx.m_flFixedCurtime;
}

void CCSGOPlayerAnimState::SetLayerSequence( CAnimationLayer* layer, int32_t activity, bool reset ) {
	int32_t iSequence{ this->SelectSequenceFromActMods( activity ) };
	if ( iSequence < 2 )
		return;

	if ( reset ) {
		layer->flCycle = 0.0f;
		layer->flWeight = 0.0f;
	}
	layer->nSequence = iSequence;
	layer->flPlaybackRate = this->m_pPlayer->GetLayerSequenceCycleRate( layer, iSequence );
}

int32_t CCSGOPlayerAnimState::SelectSequenceFromActMods( int32_t iActivity ) {
	bool bIsPlayerDucked{ flDuckAmount > 0.55f };
	bool bIsPlayerRunning{ flSpeedAsPortionOfWalkTopSpeed > 0.25f };

	int32_t iLayerSequence = -1;
	switch ( iActivity )
	{
	case ACT_CSGO_JUMP:
	{
		iLayerSequence = 15 + static_cast < int32_t >( bIsPlayerRunning );
		if ( bIsPlayerDucked )
			iLayerSequence = 17 + static_cast < int32_t >( bIsPlayerRunning );
	}
	break;

	case ACT_CSGO_ALIVE_LOOP:
	{
		iLayerSequence = 8;
		if ( pLastActiveWeapon != pActiveWeapon )
			iLayerSequence = 9;
	}
	break;

	case ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING:
	{
		iLayerSequence = 6;
	}
	break;

	case ACT_CSGO_FALL:
	{
		iLayerSequence = 14;
	}
	break;

	case ACT_CSGO_IDLE_TURN_BALANCEADJUST:
	{
		iLayerSequence = 4;
	}
	break;

	case ACT_CSGO_LAND_LIGHT:
	{
		iLayerSequence = 20;
		if ( bIsPlayerRunning )
			iLayerSequence = 22;

		if ( bIsPlayerDucked )
		{
			iLayerSequence = 21;
			if ( bIsPlayerRunning )
				iLayerSequence = 19;
		}
	}
	break;

	case ACT_CSGO_LAND_HEAVY:
	{
		iLayerSequence = 23;
		if ( bIsPlayerDucked )
			iLayerSequence = 24;
	}
	break;

	case ACT_CSGO_CLIMB_LADDER:
	{
		iLayerSequence = 13;
	}
	break;
	default: break;
	}

	return iLayerSequence;
}