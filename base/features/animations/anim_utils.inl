#pragma once
#include "../rage/exploits.h"
#include "animation.h"

inline void PlayerEntry::reset( ) {
	this->m_pRecords.clear( );

	this->m_optPreviousData.reset( );
	this->m_flSpawnTime = 0;
	//this->m_iMissedShots = 0;

	//this->m_vecUpdatedOrigin = { 0,0,0 };
	this->m_vecLastOrigin = { 0,0,0 };
	this->m_pPlayer = nullptr;

	this->m_bBrokeLC = false;

	this->m_iLastUnchoked = 0;
	this->m_iRealChoked = 0;
}

inline void LagRecord_t::FinalAdjustments( CBasePlayer* player, const std::optional<AnimData_t>& previous, int chokedReal ) {
	if ( !previous.has_value( ) )
		return;

	auto& prevLayers = previous->m_pLayers;
	auto& curLayers = this->m_cAnimData.m_pLayers;

	auto& curVel{ this->m_cAnimData.m_vecVelocity };
	auto& curWeapon{ this->m_cAnimData.m_pWeapon };

	/* choked fix */
	if ( prevLayers[ 11 ].flPlaybackRate == curLayers[ 11 ].flPlaybackRate ) {
		if ( curLayers[ 11 ].flPlaybackRate > 0.f 
			&& ( !curWeapon || curWeapon == previous->m_pWeapon )
			&& curLayers[ 11 ].flCycle > prevLayers[ 11 ].flCycle ) {
			const auto tickDifference = TIME_TO_TICKS( ( curLayers[ 11 ].flCycle - prevLayers[ 11 ].flCycle ) / curLayers[ 11 ].flPlaybackRate );

			if ( tickDifference > this->m_iNewCmds
				&& tickDifference <= 18 )
				this->m_iNewCmds = tickDifference;
		}
	}

	/* jump_fall fix */ 
	auto chokedTime{ TICKS_TO_TIME( this->m_iNewCmds ) };

	if ( this->m_iNewCmds > 1 ) {
		const auto UpdateStartTime = this->m_cAnimData.m_flSimulationTime - chokedTime;
		if ( !( this->m_cAnimData.m_iFlags & FL_ONGROUND )
			&& curLayers[ 4 ].flCycle < 0.5f 
			&& curLayers[ 4 ].flPlaybackRate > 0.f ) {

			this->m_flOnGroundTime = this->m_cAnimData.m_flSimulationTime - ( curLayers[ 4 ].flCycle / curLayers[ 4 ].flPlaybackRate );
			this->m_bLanded = this->m_flOnGroundTime >= UpdateStartTime;
		}
	}

	chokedTime = TICKS_TO_TIME( chokedReal );

	/* velo fix */
	if ( chokedTime > 0.f
		&& chokedTime < 1.f )
		curVel = ( this->m_cAnimData.m_vecOrigin - previous->m_vecOrigin ) / chokedTime;

	if ( this->m_cAnimData.m_iFlags & FL_ONGROUND && previous->m_iFlags & FL_ONGROUND ) {
		curVel.z = 0.f;

		const auto maxCurrentSpeed{ curWeapon ? std::max( 0.1f, ( player->m_bIsScoped( ) ? curWeapon->GetCSWeaponData( )->flMaxSpeedAlt : curWeapon->GetCSWeaponData( )->flMaxSpeed ) ) : player->m_flMaxSpeed( ) };

		if ( curLayers[ 6 ].flPlaybackRate == 0.f )
			curVel = { 0, 0, 0 };
		else if ( ( !curWeapon || curWeapon == previous->m_pWeapon )
			&& curLayers[ 11 ].flPlaybackRate == prevLayers[ 11 ].flPlaybackRate
			&& !player->m_flDuckAmount( ) 
			&& !player->m_bIsWalking( ) ) {

			const auto avgSpeed{ curVel.Length2D( ) };
			if ( curLayers[ 11 ].flWeight > 0.f && curLayers[ 11 ].flWeight < 1.f ) {
				const auto m_flSpeedAsPortionOfRunTopSpeed{ 0.55f + ( 1.f - curLayers[ 11 ].flWeight ) * 0.35f };

				if ( m_flSpeedAsPortionOfRunTopSpeed < 1.f && m_flSpeedAsPortionOfRunTopSpeed > 0.f ) {
					const auto m_flVelocityLengthXY{ m_flSpeedAsPortionOfRunTopSpeed * maxCurrentSpeed };

					curVel.x *= ( m_flVelocityLengthXY / avgSpeed );
					curVel.y *= ( m_flVelocityLengthXY / avgSpeed );
				}
			}
		}
	}
}

inline bool LagRecord_t::IsValid( ) {
	int delay = 0;
	if ( ctx.m_bFakeDucking )
		delay = 14 - Interfaces::ClientState->nChokedCommands;

	const auto latencyTicks = std::max( 0, TIME_TO_TICKS( ctx.m_flOutLatency ) );
	const auto serverCurtime = TICKS_TO_TIME( delay + Interfaces::ClientState->iServerTick + latencyTicks );
	const int fldeadTime = serverCurtime - Offsets::Cvars.sv_maxunlag->GetFloat( );

	if ( this->m_cAnimData.m_flSimulationTime < fldeadTime )
		return false;

	const auto ticksAllowed = ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) && Features::Exploits.m_iRechargeCmd != Interfaces::ClientState->iLastOutgoingCommand ? ctx.m_iTicksAllowed : 0;
	const auto correct = std::clamp( ctx.m_flOutLatency + ctx.m_flInLatency + ctx.m_flLerpTime, 0.f, Offsets::Cvars.sv_maxunlag->GetFloat( ) );
	const auto delta = correct - ( TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) - ticksAllowed ) - this->m_cAnimData.m_flSimulationTime );
	return std::abs( delta ) < 0.2f;
}

inline void LagRecord_t::Apply( CBasePlayer* ent, bool onlyAnim ) {
	if ( !onlyAnim ) {
		ent->m_vecOrigin( ) = this->m_cAnimData.m_vecOrigin;
		ent->SetAbsOrigin( this->m_cAnimData.m_vecOrigin );

		ent->SetCollisionBounds(
			this->m_cAnimData.m_vecMins, this->m_cAnimData.m_vecMaxs
		);
	}

	ent->SetAbsAngles( { 0.f, this->m_flAbsYaw, 0.f } );

	memcpy( ent->m_CachedBoneData( ).Base( ), this->m_pMatrix, this->m_iBonesCount * sizeof( matrix3x4_t ) );

	ent->g_iModelBoneCounter( ) = **reinterpret_cast< unsigned long** >( Offsets::Sigs.InvalidateBoneCache + 0xau );
}

FORCEINLINE void LagBackup_t::Apply( CBasePlayer* ent ) {
	ent->m_vecOrigin( ) = this->m_vecOrigin;
	ent->SetAbsOrigin( this->m_vecAbsOrigin );

	ent->SetCollisionBounds(
		this->m_vecMins, this->m_vecMaxs
	);

	ent->SetAbsAngles( { 0.f, this->m_flAbsYaw, 0.f } );

	auto& bone_accessor = ent->m_BoneAccessor( );

	bone_accessor.nWritableBones = this->m_iWriteableBones;
	bone_accessor.nReadableBones = this->m_iReadableBones;

	memcpy( ent->m_CachedBoneData( ).Base( ), this->m_matMatrix, this->m_iBonesCount * sizeof( matrix3x4_t ) );

	ent->g_iModelBoneCounter( ) = this->m_lModelBoneCounter;
}

FORCEINLINE LagBackup_t::LagBackup_t( CBasePlayer* ent ) {
	this->m_vecOrigin = ent->m_vecOrigin( );
	this->m_vecAbsOrigin = ent->GetAbsOrigin( );

	this->m_vecMins = ent->m_vecMins( );
	this->m_vecMaxs = ent->m_vecMaxs( );

	this->m_lModelBoneCounter = ent->g_iModelBoneCounter( );

	const auto& bone_accessor = ent->m_BoneAccessor( );

	this->m_iReadableBones = bone_accessor.nReadableBones;
	this->m_iWriteableBones = bone_accessor.nWritableBones;

	this->m_iBonesCount = ent->m_CachedBoneData( ).Count( );
	this->m_flAbsYaw = ent->m_pAnimState( )->flAbsYaw;

	memcpy( this->m_matMatrix, ent->m_CachedBoneData( ).Base( ), this->m_iBonesCount * sizeof( matrix3x4_t ) );
}