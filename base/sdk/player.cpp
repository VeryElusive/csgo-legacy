#include "entity.h"
#include "../context.h"
#include "../features/rage/exploits.h"

bool CBasePlayer::IsTeammate( CBasePlayer* player ) {
	if ( !player )
		player = ctx.m_pLocal;

	if ( !player )
		return false;

	if ( Offsets::Cvars.mp_teammates_are_enemies != nullptr
		&& Offsets::Cvars.mp_teammates_are_enemies->GetBool( ) && this != ctx.m_pLocal ) // you see that guy over there? yeah. he's not ur friend
		return false;

	return ( this->m_iTeamNum( ) == player->m_iTeamNum( ) );
}

int CBasePlayer::GetSequenceActivity( int sequence ) {
	studiohdr_t* pStudioHdr = Interfaces::ModelInfo->GetStudioModel( this->GetModel( ) );
	if ( pStudioHdr == nullptr )
		return -1;

	using GetSequenceActivityFn = int( __fastcall* )( void*, void*, int );
	static auto oGetSequenceActivity = ( GetSequenceActivityFn )Offsets::Sigs.GetSequenceActivity;
	return oGetSequenceActivity( this, pStudioHdr, sequence );
}

bool CBasePlayer::IsDead( ) {
	if ( m_iHealth( ) <= 0 )
		return true;

	return ( m_lifeState( ) );
}

CWeaponCSBase* CBasePlayer::GetWeapon( ) {
	return static_cast< CWeaponCSBase * >( Interfaces::ClientEntityList->GetClientEntityFromHandle( this->m_hActiveWeapon( ) ) );
}

Vector CBasePlayer::GetEyePosition( float pitch ) {
	auto v12 = this->m_pAnimState( );
	if ( !v12 )
		return { };

	const auto backupPoseParam{ ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) };

	ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = ( std::clamp( std::remainder(
		pitch
		- ctx.m_pLocal->m_aimPunchAngle( ).x * Offsets::Cvars.weapon_recoil_scale->GetFloat( ), 360.f
	), -90.f, 90.f ) + 90.f ) / 180.f;

	// idk if this is necessary but ill do it anyways
	this->SetAbsAngles( { 0.f, v12->flAbsYaw, 0.f } );

	Features::AnimSys.SetupBonesFixed( this, ctx.m_matRealLocalBones, BONE_USED_BY_HITBOX, Interfaces::Globals->flCurTime, ( INVALIDATEBONECACHE | SETUPBONESFRAME | NULLIK | OCCLUSIONINTERP ) );

	auto eyePos{ this->m_vecOrigin( ) + this->m_vecViewOffset( ) };

	if ( v12->pEntity && ( v12->bHitGroundAnimation || v12->flDuckAmount != 0.f || this->m_hGroundEntity( ) == -1 ) ) {
		static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Offsets::Sigs.LookupBone ) };
		const auto boneIndex{ lookupBone( v12->pEntity, _( "head_0" ) ) };

		if ( boneIndex != -1 ) {
			Vector headPos{
				ctx.m_matRealLocalBones[ boneIndex ][ 0u ][ 3u ],
				ctx.m_matRealLocalBones[ boneIndex ][ 1u ][ 3u ],
				ctx.m_matRealLocalBones[ boneIndex ][ 2u ][ 3u ] + 1.7f
			};

			if ( eyePos.z > headPos.z ) {
				const auto v5{ std::abs( eyePos.z - headPos.z ) };
				const auto v6{ std::max( ( v5 - 4.f ) / 6.f, 0.f ) };
				const auto v7{ std::min( v6, 1.f ) };

				eyePos.z += ( ( ( v7 * v7 ) * 3.f ) - ( ( v7 + v7 ) * ( v7 * v7 ) ) ) * ( headPos.z - eyePos.z );
			}
		}
	}

	ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = backupPoseParam;

	return eyePos;
}

bool CBasePlayer::IsHostage( ) {
	auto client = this->GetClientClass( );
	return client->nClassID == EClassIndex::CHostage;
}

bool CBasePlayer::CanShoot( ) {
	if ( this->m_fFlags( ) & 0x40 )
		return false;

	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) )
		return false;

	const auto weapon{ this->GetWeapon( ) };
	if ( !weapon )
		return false;

	if ( this->m_bWaitForNoAttack( ) )
		return false;

	if ( this->m_iPlayerState( ) )
		return false;

	if ( this->m_bIsDefusing( ) )
		return false;

	auto tickbase = this->m_nTickBase( );
	const int ticksAllowed{ ctx.m_iTicksAllowed };
	const bool properWeap{ ctx.m_pWeaponData->nWeaponType > WEAPONTYPE_KNIFE && ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4 };
	if ( properWeap )
		tickbase -= ticksAllowed;

	const float curtime = TICKS_TO_TIME( tickbase );
	if ( curtime < this->m_flNextAttack( ) )
		return false;

	if ( weapon->m_iItemDefinitionIndex( ) == WEAPON_C4 )
		return true;

	if ( ( properWeap || weapon->m_iItemDefinitionIndex( ) == WEAPON_TASER )
		&& ( weapon->m_iClip1( ) <= 0 || weapon->m_bReloading( ) )
		|| weapon->m_flNextPrimaryAttack( ) > curtime )
		return false;

	if ( weapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER )
		return true;

	if ( weapon->m_nSequence( ) != 5 )
		return false;

	return curtime >= weapon->m_flPostponeFireReadyTime( );
}