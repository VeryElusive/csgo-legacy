#include "animation.h"

// https://www.youtube.com/watch?v=a3Z7zEc7AXQ
void CAnimationSys::RunAnimationSystem( ) {
	const int latencyTicks = std::max( 0, TIME_TO_TICKS( ctx.m_flRealOutLatency ) );
	const float serverCurtime = TICKS_TO_TIME( Interfaces::ClientState->iServerTick + latencyTicks );
	const int flDeadtime = serverCurtime - Offsets::Cvars.sv_maxunlag->GetFloat( );

	for ( int i = 1; i <= 64; i++ ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player || !player->IsPlayer( ) || player == ctx.m_pLocal )
			continue;

		auto& entry = m_arrEntries.at( i - 1 );

		if ( entry.m_pPlayer != player )
			entry.reset( );

		entry.m_pPlayer = player;

		if ( player->IsDormant( ) || player->IsDead( ) ) {
			entry.reset( );
			continue;
		}

		if ( player->m_iAnimationLayersCount( ) <= 0 )
			continue;

		const auto state{ player->m_pAnimState( ) };
		if ( !state ) {
			entry.reset( );

			continue;
		}

		if ( entry.m_optPreviousData.has_value( )
			&& entry.m_optPreviousData->m_pLayers[ 11 ].flCycle == player->m_AnimationLayers( )[ 11 ].flCycle ) {
			player->m_flSimulationTime( ) = player->m_flOldSimulationTime( );
			continue;
		}

		if ( player->m_flSpawnTime( ) != entry.m_flSpawnTime ) {
			state->Reset( );
			entry.m_pRecords.clear( );
			entry.m_optPreviousData.reset( );
			entry.m_flSpawnTime = player->m_flSpawnTime( );
		}

		/*if ( !entry.m_pRecords.empty( ) ) {
			const auto& lastRecord{ entry.m_pRecords.back( ) };
			const auto jitter{ lastRecord->m_angEyeAngles.y - player->m_angEyeAngles( ).y };

			const auto jitterAvg{ ( std::abs( jitter ) + std::abs( entry.m_flJitterAmount ) ) / 2 };

			entry.m_flJitterAmount = std::copysign( jitterAvg, jitter );
		}*/

		bool wasRecordAddedToTrack{ true };

		if ( Config::Get<bool>( Vars.RagebotLagcompensation ) ) {
			if ( entry.m_flTrackSimulationTime >= player->m_flSimulationTime( ) ) {
				if ( entry.m_pRecords.empty( )
					|| !entry.m_pRecords.back( )->m_bBrokeLC ) {
					const auto rec{ std::make_unique< LagRecord_t >( player ) };
					rec->FinalAdjustments( player, entry.m_optPreviousData );

					AnimatePlayer( rec.get( ), entry );

					entry.m_optPreviousData = rec->m_cAnimData;
					entry.m_iLastChoked = rec->m_iNewCmds;

					continue;
				}

				wasRecordAddedToTrack = false;
			}
		}

		if ( player->IsTeammate( ) ) {
			while ( entry.m_pRecords.size( ) )
				entry.m_pRecords.pop_front( );
		}
		else {
			entry.m_pRecords.erase(
				std::remove_if(
					entry.m_pRecords.begin( ), entry.m_pRecords.end( ),
					[ & ]( const std::shared_ptr< LagRecord_t >& lag_record ) -> bool {
						return lag_record->m_cAnimData.m_flSimulationTime < flDeadtime;
					}
				),
				entry.m_pRecords.end( )
						);
		}

		entry.m_bBrokeLC = ( player->m_vecOrigin( ) - entry.m_vecLastOrigin ).LengthSqr( ) > 4096.f;
		if ( entry.m_bBrokeLC && !entry.m_pRecords.empty( ) )
			entry.m_pRecords.clear( );

		// NOW we add record
		const auto current{ entry.m_pRecords.emplace_back( 
			std::make_shared< LagRecord_t >( player ) ).get( ) };

		current->FinalAdjustments( player, entry.m_optPreviousData );

		AnimatePlayer( current, entry );

		if ( wasRecordAddedToTrack ) {
			entry.m_flTrackSimulationTime = current->m_cAnimData.m_flSimulationTime;
			entry.m_vecLastOrigin = current->m_cAnimData.m_vecOrigin;
			current->m_bBrokeLC = entry.m_bBrokeLC;
		}
		else
			current->m_bBrokeLC = true;// just using this for broke lc flag in player esp

		entry.m_optPreviousData = current->m_cAnimData;
		entry.m_iLastChoked = current->m_iNewCmds;
	}
}

// TODO: do this shit for local player, atm i really cannot be fucked
void CAnimationSys::UpdatePlayerMatrixes( ) {
	if ( !ctx.m_pLocal )
		return;

	for ( int i = 1; i <= 64; i++ ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player || !player->IsPlayer( ) || player->Dormant( ) || player == ctx.m_pLocal )
			continue;

		auto& entry = m_arrEntries.at( i - 1 );
		if ( entry.m_pPlayer != player )
			continue;

		const auto delta{ player->GetAbsOrigin( ) - entry.m_vecUpdatedOrigin };
		if ( !delta.Length( ) )
			continue;

		/*for ( std::size_t i{ }; i < player->m_CachedBoneData( ).Count( ); ++i ) {
			auto& bone{ player->m_CachedBoneData( ).Base( )[ i ] };
			auto& mat{ entry.m_matMatrix[ i ] };
			bone[ 0 ][ 3 ] = mat[ 0 ][ 3 ] + delta.x;
			bone[ 1 ][ 3 ] = mat[ 1 ][ 3 ] + delta.y;
			bone[ 2 ][ 3 ] = mat[ 2 ][ 3 ] + delta.z;
		}*/

		//ctx.m_bSetupBones = ctx.m_bClampbones = true;
		//player->SetupBones( nullptr, 256, BONE_USED_BY_ANYTHING, Interfaces::Globals->flCurTime );
		//ctx.m_bSetupBones = ctx.m_bClampbones = false;
	}
}

void CAnimationSys::AnimatePlayer( LagRecord_t* current, PlayerEntry& entry ) {
	struct anim_backup_t {
		__forceinline constexpr anim_backup_t( ) = default;

		__forceinline anim_backup_t( CBasePlayer* const player )
			: m_anim_state{ *player->m_pAnimState( ) }, m_abs_yaw{ m_anim_state.flAbsYaw },
			m_pose_params{ player->m_flPoseParameter( ) }
		{ memcpy( m_anim_layers, player->m_AnimationLayers( ), 0x38 * player->m_iAnimationLayersCount( ) ); }
			
		__forceinline void restore( CBasePlayer* const player ) const {
			*player->m_pAnimState( ) = m_anim_state;

			player->SetAbsAngles( { 0.f, m_abs_yaw, 0.f } );

			memcpy( player->m_AnimationLayers( ), m_anim_layers, 0x38 * player->m_iAnimationLayersCount( ) );
			player->m_flPoseParameter( ) = m_pose_params;
		}

		CCSGOPlayerAnimState		m_anim_state{ };

		float					m_abs_yaw{ }, m_lby{ };

		CAnimationLayer m_anim_layers[ 13 ];
		std::array<float, 24>	m_pose_params{ };
	} anim_backup{ entry.m_pPlayer };

	entry.m_pPlayer->SetAbsOrigin( current->m_cAnimData.m_vecOrigin );

	UpdateSide( entry, current, 0 );

	current->m_bAnimated = true;

	entry.m_vecUpdatedOrigin = entry.m_pPlayer->GetAbsOrigin( );

	std::memcpy( entry.m_pPlayer->m_AnimationLayers( ), current->m_cAnimData.m_pLayers, 0x38 * entry.m_pPlayer->m_iAnimationLayersCount( ) );
}

void CAnimationSys::UpdateSide( PlayerEntry& entry, LagRecord_t* current, int side ) {
	const auto state{ entry.m_pPlayer->m_pAnimState( ) };
	if ( !state )
		return;

	const auto backupCurtime{ Interfaces::Globals->flCurTime };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };
	const auto backupHLTV{ Interfaces::ClientState->bIsHLTV };

	Interfaces::Globals->flCurTime = current->m_cAnimData.m_flSimulationTime;
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

	if ( entry.m_optPreviousData.has_value( ) ) {
		const auto& layer6{ entry.m_optPreviousData->m_pLayers[ 6 ] };
		const auto& layer12{ entry.m_optPreviousData->m_pLayers[ 12 ] };
		const auto& layer7{ entry.m_optPreviousData->m_pLayers[ 7 ] };

		state->flFeetCycle == layer6.flCycle;
		state->flMoveWeight == layer6.flWeight;
		if ( layer6.flPlaybackRate > 0.f 
			&& state->flDurationMoving <= 0.f )
			state->flMoveWeight = 0.f;

		state->iStrafeSequence == layer7.nSequence;
		state->flStrafeWeight == layer7.flWeight;
		state->flStrafeCycle == layer7.flCycle;
	}
	else {
		if ( current->m_cAnimData.m_iFlags & FL_ONGROUND )
			state->bHitGroundAnimation = false;

		state->flLastUpdateTime = current->m_cAnimData.m_flSimulationTime - Interfaces::Globals->flIntervalPerTick;
	}

	entry.m_pPlayer->SetAbsVelocity( current->m_cAnimData.m_vecVelocity );

	if ( current->m_bLanded.has_value( ) )
		state->flDurationInAir = std::max( current->m_cAnimData.m_flSimulationTime - TICKS_TO_TIME( 1 ) - current->m_flOnGroundTime, 0.f );

	if ( side == 1 )
		state->flAbsYaw = entry.m_pPlayer->m_angEyeAngles( ).y + 120.f;
	else if ( side == 2 )
		state->flAbsYaw = entry.m_pPlayer->m_angEyeAngles( ).y - 120.f;

	state->flAbsYaw = std::remainderf( state->flAbsYaw, 360.f );

	entry.m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = Interfaces::ClientState->bIsHLTV = true;
	entry.m_pPlayer->UpdateClientsideAnimations( );
	entry.m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

	Interfaces::ClientState->bIsHLTV = backupHLTV;
	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;

	current->m_cAnimData.m_flPoseParameter = entry.m_pPlayer->m_flPoseParameter( );
	current->m_cAnimData.m_flAbsYaw = state->flAbsYaw;
	std::memcpy( current->m_cAnimData.m_pLayers, entry.m_pPlayer->m_AnimationLayers( ), 0x38 * entry.m_pPlayer->m_iAnimationLayersCount( ) );

	std::memcpy( entry.m_pPlayer->m_AnimationLayers( ), current->m_cAnimData.m_pLayers, 0x38 * entry.m_pPlayer->m_iAnimationLayersCount( ) );
	entry.m_pPlayer->SetAbsAngles( { 0.f, state->flAbsYaw, 0.f } );

	SetupBonesFixed( entry.m_pPlayer, current->m_cAnimData.m_pMatrix, BONE_USED_BY_SERVER,
		current->m_cAnimData.m_flSimulationTime, USEALLSETUPBONESFLAGS, &current->m_cAnimData.m_cIk );

	const auto mdlData{ entry.m_pPlayer->m_pStudioHdr( ) };
	if ( !mdlData
		|| !mdlData->pStudioHdr )
		return;

	current->m_iBonesCount = mdlData->pStudioHdr->nBones;

	if ( current->m_iBonesCount > 256 )
		current->m_iBonesCount = 256;
}

bool CAnimationSys::SetupBonesFixed( CBasePlayer* const player, matrix3x4_t bones[ 256 ], const int mask, const float time, const int flags, CIKContext* ik ) {
	const auto backupCurtime{ Interfaces::Globals->flCurTime };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };
	const auto backupFramecount{ Interfaces::Globals->iFrameCount };
	const auto backupOcclusionFrame{ player->m_iOcclusionFrame( ) };
	const auto backupOcclusionFlags{ player->m_iOcclusionFlags( ) };
	const auto backupEntClientFlags{ player->m_iEntClientFlags( ) };
	const auto backupMaintainSequenceTransitions{ player->m_bMaintainSequenceTransitions( ) };
	const auto backupPrevBoneMask{ player->m_iPrevBoneMask( ) };
	const auto backupAccumulatedBoneMask{ player->m_iAccumulatedBoneMask( ) };
	const auto backupEffects{ player->m_fEffects( ) };
	const auto backupIK{ player->m_IkContext( ) };

	Interfaces::Globals->flCurTime = time;
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

	if ( flags & INVALIDATEBONECACHE ) {
		player->g_iModelBoneCounter( ) = 0ul;
		player->m_flLastSetupBonesTime( ) = std::numeric_limits< float >::lowest( );

		auto& boneAccessor{ player->m_BoneAccessor( ) };
		boneAccessor.nWritableBones = boneAccessor.nReadableBones = 0;
	}

	if ( flags & SETUPBONESFRAME ) {
		// i don't think i need to back this up
		player->m_iAccumulatedBoneMask( ) = 0;
		player->m_iPrevBoneMask( ) = 0;
		player->m_iLastSetupBonesFrame( ) = 0;
	}

	if ( flags & CLAMPBONESINBBOX )
		ctx.m_bServerSetupbones = true;

	if ( flags & NULLIK ) {
		player->m_IkContext( ) = nullptr;
		player->m_iEntClientFlags( ) |= 2u;
	}

	if ( flags & OCCLUSIONINTERP ) {
		player->m_fEffects( ) |= 8u;
		player->m_bMaintainSequenceTransitions( ) = false;

		// C_CSPlayer::ReevauluateAnimLOD and C_CSPlayer::AccumulateLayers
		player->m_iOcclusionFlags( ) &= ~0xau;
		player->m_iOcclusionFrame( ) = 0;
	}

	ctx.m_bSetupBones = true;
	const auto ret = player->SetupBones( bones, 256, mask, time );
	ctx.m_bSetupBones = ctx.m_bServerSetupbones = false;

	if ( ik && player->m_IkContext( ) )
		*ik = *player->m_IkContext( );

	if ( flags & NULLIK ) {
		player->m_IkContext( ) = backupIK;
		player->m_iEntClientFlags( ) = backupEntClientFlags;
	}

	if ( flags & SETUPBONESFRAME ) {
		player->m_iAccumulatedBoneMask( ) = backupAccumulatedBoneMask;
		player->m_iPrevBoneMask( ) = backupPrevBoneMask;
	}

	if ( flags & OCCLUSIONINTERP ) {
		player->m_fEffects( ) = backupEffects;
		player->m_iOcclusionFlags( ) = backupOcclusionFlags;
		player->m_iOcclusionFrame( ) = backupOcclusionFrame;
		player->m_bMaintainSequenceTransitions( ) = backupMaintainSequenceTransitions;
	}

	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;
	Interfaces::Globals->iFrameCount = backupFramecount;

	ctx.m_bServerSetupbones = false;

	return true;
}