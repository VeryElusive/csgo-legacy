#pragma once
#include "../rage/exploits.h"
#include "animation.h"

inline void PlayerEntry::reset( ) {
	this->m_pRecords.clear( );
	this->m_pInterpolatedData.clear( );

	this->m_optPreviousData.reset( );
	this->m_flSpawnTime = 0;
	this->m_iMissedShots = 0;

	this->m_vecUpdatedOrigin = { 0,0,0 };
	this->m_pPlayer = nullptr;

	this->m_bBrokeLC = false;
}

inline void LagRecord_t::FinalAdjustments( CBasePlayer* player, std::optional<AnimData_t>& previous ) {
	if ( !previous.has_value( ) )
		return;

	auto& prevLayers = previous->m_pLayers;
	auto& curLayers = this->m_cAnimData.m_pLayers;

	auto& curVel{ this->m_cAnimData.m_vecVelocity };
	auto& curWeapon{ this->m_cAnimData.m_pWeapon };

	/* choked fix */
	if ( prevLayers[ 11 ].flPlaybackRate == curLayers[ 11 ].flPlaybackRate ) {
		if ( this->m_cAnimData.m_pWeapon == previous->m_pWeapon
			&& curLayers[ 11 ].flPlaybackRate > 0.f 
			&& curLayers[ 11 ].flCycle > prevLayers[ 11 ].flCycle ) {
			const auto tickDifference = TIME_TO_TICKS( ( curLayers[ 11 ].flCycle - prevLayers[ 11 ].flCycle ) / curLayers[ 11 ].flPlaybackRate );

			if ( tickDifference > this->m_iNewCmds
				&& tickDifference <= 18 )
				this->m_iNewCmds = tickDifference;
		}
	}

	/* jump_fall fix */
	const auto chokedTime = TICKS_TO_TIME( this->m_iNewCmds );

	if ( this->m_iNewCmds > 1 ) {
		const auto UpdateStartTime = this->m_cAnimData.m_flSimulationTime - chokedTime;
		if ( !( this->m_cAnimData.m_iFlags & FL_ONGROUND )
			&& curLayers[ 4 ].flCycle < 0.5f 
			&& curLayers[ 4 ].flPlaybackRate > 0.f ) {

			this->m_flOnGroundTime = this->m_cAnimData.m_flSimulationTime - ( curLayers[ 4 ].flCycle / curLayers[ 4 ].flPlaybackRate );
			this->m_bLanded = this->m_flOnGroundTime >= UpdateStartTime;
		}
	}


	/* velo fix */
	if ( chokedTime > 0.f
		&& chokedTime < 1.f )
		curVel = ( this->m_cAnimData.m_vecOrigin - previous->m_vecOrigin ) / chokedTime;

	if ( this->m_cAnimData.m_iFlags & FL_ONGROUND && previous->m_iFlags & FL_ONGROUND ) {
		if ( curLayers[ 6 ].flPlaybackRate == 0.f )
			curVel = { 0, 0, 0 };
		else if ( curWeapon
			&& curWeapon == previous->m_pWeapon
			&& curLayers[ 11 ].flPlaybackRate == prevLayers[ 11 ].flPlaybackRate
			&& this->m_iNewCmds > 1 ) {
			const auto maxCurrentSpeed{ std::max( 0.1f, ( player->m_bIsScoped( ) ? curWeapon->GetCSWeaponData( )->flMaxSpeedAlt : curWeapon->GetCSWeaponData( )->flMaxSpeed ) ) };

			const auto speed{ ( 0.55f - ( ( curLayers[ 11 ].flWeight - 1.f ) * 0.35f ) ) * maxCurrentSpeed };
			const auto avgSpeed = curVel.Length2D( );

			if ( curLayers[ 11 ].flWeight < 1.f ) {
				if ( curLayers[ 11 ].flWeight <= 0.f && speed <= avgSpeed )
					return;
			}
			else {
				if ( avgSpeed <= speed )
					return;
			}

			if ( avgSpeed != 0.f ) {
				curVel.y /= avgSpeed;
				curVel.x /= avgSpeed;
			}

			curVel.x *= speed;
			curVel.y *= speed;

			//ctx.m_strDbgLogs.push_back( std::make_pair( "X: " + std::to_string( curVel.x ) + " Y: " + std::to_string( curVel.y ), player ) );
		}
	}

	/*if ( this->m_cAnimData.m_iFlags & FL_ONGROUND
		&& this->m_iNewCmds > 1 ) {
		const auto average_speed = curVel.Length2D( );
		if ( average_speed > 0.1f ) {
			const auto alive_loop_weight = curLayers[ 11 ].flWeight;
			if ( alive_loop_weight > 0.f
				&& alive_loop_weight < 1.f ) {
				const auto unk = ( 1.f - alive_loop_weight ) * 0.35f;
				if ( unk > 0.f
					&& unk < 1.f ) {
					const auto modifier = ( ( unk + 0.55f ) * maxCurrentSpeed ) / average_speed;

					curVel.x *= modifier;
					curVel.y *= modifier;
					
				}
			}

			curVel.z = 0.f;
		}
	}*/
}

inline void LagRecord_t::SelectResolverSide( PlayerEntry& entry ) {
	if ( !this->m_bMultiMatrix )
		return;

	if ( !entry.m_iMissedShots ) {
		if ( std::abs( entry.m_pPlayer->m_angEyeAngles( ).y - this->m_cAnimData.m_cAnimSides.at( LEFT ).m_flAbsYaw )
			< std::abs( entry.m_pPlayer->m_angEyeAngles( ).y - this->m_cAnimData.m_cAnimSides.at( RIGHT ).m_flAbsYaw ) )
			this->m_iResolverSide = LEFT;
		else
			this->m_iResolverSide = RIGHT;

		if ( std::abs( entry.m_pPlayer->m_angEyeAngles( ).y - this->m_cAnimData.m_cAnimSides.at( this->m_iResolverSide ).m_flAbsYaw )
			> std::abs( entry.m_pPlayer->m_angEyeAngles( ).y - this->m_cAnimData.m_cAnimSides.at( MIDDLE ).m_flAbsYaw ) )
			this->m_iResolverSide = MIDDLE;
	}
	else {
		Features::AnimSys.GetSide( entry );
		this->m_iResolverSide = entry.m_iResolverSide;
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
	return std::fabsf( delta ) <= 0.2f;
}

inline void LagRecord_t::Apply( CBasePlayer* ent, bool onlyAnim ) {
	if ( !onlyAnim ) {
		ent->m_vecOrigin( ) = this->m_cAnimData.m_vecOrigin;
		ent->SetAbsOrigin( this->m_cAnimData.m_vecOrigin );

		ent->SetCollisionBounds(
			this->m_vecMins, this->m_vecMaxs
		);
	}

	const auto& side = this->m_cAnimData.m_cAnimSides.at( this->m_iResolverSide );

	ent->SetAbsAngles( { 0.f, side.m_flAbsYaw, 0.f } );

	memcpy( ent->m_CachedBoneData( ).Base( ), side.m_pMatrix, side.m_iBonesCount * sizeof( matrix3x4_t ) );

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