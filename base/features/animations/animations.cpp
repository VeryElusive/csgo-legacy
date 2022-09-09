#include "animation.h"

// https://www.youtube.com/watch?v=a3Z7zEc7AXQ
void CAnimationSys::RunAnimationSystem( ) {
	const int latencyTicks = std::max( 0, TIME_TO_TICKS( ctx.m_flOutLatency ) );
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

		/*if ( player->IsDormant( ) ) {
			if ( entry.m_pRecords.empty( ) ) {
				entry.m_pRecords.emplace_back( std::make_shared< LagRecord_t >( player ) );
				continue;
			}

			if ( !entry.m_pRecords.back( )->m_bDormant ) {
				entry.m_pRecords.clear( );
				entry.m_pRecords.emplace_back( std::make_shared< LagRecord_t >( player ) );
				continue;
			}

			continue;
		}*/

		const auto state = player->m_pAnimState( );
		if ( !state ) {
			entry.reset( );

			continue;
		}

		if ( entry.m_optPreviousData.has_value( ) ) {
			if ( entry.m_optPreviousData->m_pLayers[ 11 ].flCycle == player->m_AnimationLayers( )[ 11 ].flCycle ) {
				player->m_flSimulationTime( ) = player->m_flOldSimulationTime( );
				continue;
			}
		}

		entry.m_iRealChoked = Interfaces::ClientState->iServerTick - entry.m_iLastUnchoked;
		entry.m_iLastUnchoked = Interfaces::ClientState->iServerTick;

		// now while i dont want this record, or to even animate it, i still want to store the prev data for me to use in FinalAdjustments next time i animate
		if ( entry.m_optPreviousData.has_value( )
			&& entry.m_optPreviousData->m_flSimulationTime >= player->m_flSimulationTime( ) ) {
			const auto prevSimTime{ entry.m_optPreviousData->m_flSimulationTime };
			{
				const auto rec{ std::make_unique< LagRecord_t >( player ) };
				rec->FinalAdjustments( player, entry.m_optPreviousData, entry.m_iRealChoked );

				entry.m_bBrokeLC = ( rec->m_cAnimData.m_vecOrigin - entry.m_vecLastOrigin ).LengthSqr( ) > 4096.f;

				AnimatePlayer( rec.get( ), entry );

				entry.m_optPreviousData = rec->m_cAnimData;
			}
			entry.m_optPreviousData->m_flSimulationTime = prevSimTime;

			continue;
		}


		if ( player->m_flSpawnTime( ) != entry.m_flSpawnTime ) {
			state->Reset( );
			entry.m_pRecords.clear( );
			entry.m_optPreviousData.reset( );
			entry.m_flSpawnTime = player->m_flSpawnTime( );
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
						return lag_record->m_cAnimData.m_flSimulationTime < flDeadtime 
							|| std::abs( Interfaces::ClientState->iServerTick - lag_record->m_iReceiveTick ) > static_cast<int>( 0.5f / Interfaces::Globals->flIntervalPerTick );
					}
				),
				entry.m_pRecords.end( )
						);
		}

		// NOW we add record
		const auto current{ entry.m_pRecords.emplace_back( std::make_shared< LagRecord_t >( player ) ).get( ) };
		current->FinalAdjustments( player, entry.m_optPreviousData, entry.m_iRealChoked );

		entry.m_bBrokeLC = current->m_bBrokeLC = ( current->m_cAnimData.m_vecOrigin - entry.m_vecLastOrigin ).LengthSqr( ) > 4096.f;

		AnimatePlayer( current, entry );

		entry.m_vecLastOrigin = current->m_cAnimData.m_vecOrigin;

		entry.m_optPreviousData = current->m_cAnimData;
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

		const auto delta = player->GetAbsOrigin( ) - entry.m_vecUpdatedOrigin;
		for ( std::size_t i{ }; i < player->m_CachedBoneData( ).Count( ); ++i ) {
			auto& bone = player->m_CachedBoneData( ).Base( )[ i ];
			auto& mat = entry.m_matMatrix[ i ];
			bone[ 0 ][ 3 ] = mat[ 0 ][ 3 ] + delta.x;
			bone[ 1 ][ 3 ] = mat[ 1 ][ 3 ] + delta.y;
			bone[ 2 ][ 3 ] = mat[ 2 ][ 3 ] + delta.z;
		}

		//player->SetupBones_AttachmentHelper( );

		/*ctx.m_bSetupBones = true;
		player->SetupBones( entry.m_matMatrix, 256, BONE_USED_BY_ANYTHING, Interfaces::Globals->flCurTime );
		ctx.m_bSetupBones = false;*/
	}
}

void CAnimationSys::AnimatePlayer( LagRecord_t* current, PlayerEntry& entry ) {
	entry.m_pPlayer->SetAbsOrigin( current->m_cAnimData.m_vecOrigin );

	SetupInterp( current, entry );
	{
		UpdateSide( entry, current );
	}
	entry.m_pInterpolatedData.clear( );

	current->m_bAnimated = true;

	ctx.m_bSetupBones = true;
	entry.m_pPlayer->SetupBones( entry.m_matMatrix, 256, BONE_USED_BY_ANYTHING, Interfaces::Globals->flCurTime );
	ctx.m_bSetupBones = false;

	entry.m_vecUpdatedOrigin = entry.m_pPlayer->GetAbsOrigin( );

	memcpy( entry.m_pPlayer->m_AnimationLayers( ), current->m_cAnimData.m_pLayers, 0x38 * 13u );
}

FORCEINLINE void CAnimationSys::SetupInterp( LagRecord_t* to, PlayerEntry& entry ) {
	const auto& from{ entry.m_optPreviousData };
	const auto& animData{ to->m_cAnimData };

	if ( !from.has_value( ) || entry.m_iRealChoked <= 1 || entry.m_iRealChoked >= 20 ) {
		entry.m_pInterpolatedData.emplace_back( animData.m_flSimulationTime, animData.m_flDuckAmount,
			animData.m_iFlags, animData.m_vecVelocity );

		return;
	}

	entry.m_pInterpolatedData.reserve( entry.m_iRealChoked );

	// this still stays as newcmds as the left over commands still advance real choked
	const auto animTime{ animData.m_flSimulationTime - TICKS_TO_TIME( to->m_iNewCmds ) };

	const auto duckAmountDelta{ animData.m_flDuckAmount - from->m_flDuckAmount };
	const auto velocityDelta{ animData.m_vecVelocity - from->m_vecVelocity };

	const auto interpolateVelocity{
		animData.m_pLayers[ 6 ].flPlaybackRate == 0.f || from->m_pLayers[ 6 ].flPlaybackRate == 0.f
		|| animData.m_vecVelocity.Length2D( ) >= 1.1f || from->m_vecVelocity.Length2D( ) >= 1.1f };

	bool landed{ };

	// we use real choked as that is what the server uses
	for ( auto i = 1; i <= entry.m_iRealChoked; ++i ) {
		if ( i == entry.m_iRealChoked ) {
			entry.m_pInterpolatedData.emplace_back(
				animData.m_flSimulationTime, animData.m_flDuckAmount,
				animData.m_iFlags, animData.m_vecVelocity );

			break;
		}

		const auto lerp{ i / static_cast< float >( entry.m_iRealChoked ) };

		auto& interpolated{ entry.m_pInterpolatedData.emplace_back( ) };

		interpolated.m_flSimulationTime = animTime + TICKS_TO_TIME( i );
		interpolated.m_flDuckAmount = from->m_flDuckAmount + duckAmountDelta * lerp;

		interpolated.m_iFlags = animData.m_iFlags;

		if ( to->m_bLanded.has_value( ) ) {
			interpolated.m_iFlags &= ~FL_ONGROUND;
			if ( !landed ) {
				if ( interpolated.m_flSimulationTime >= to->m_flOnGroundTime ) {
					interpolated.m_iFlags |= FL_ONGROUND;
					landed = true;
				}
			}
		}

		if ( interpolateVelocity )
			interpolated.m_vecVelocity = from->m_vecVelocity + velocityDelta * lerp;
		else
			interpolated.m_vecVelocity = { ( i & 1 ) ? 1.1f : -1.1f, 0.f, 0.f };
	}
}

void CAnimationSys::UpdateSide( PlayerEntry& entry, LagRecord_t* current ) {
	const auto backupEflags{ entry.m_pPlayer->m_iEFlags( ) };
	const auto backupFlags{ entry.m_pPlayer->m_fFlags( ) };
	const auto backupVelocity{ entry.m_pPlayer->m_vecVelocity( ) };
	const auto backupAbsVelocity{ entry.m_pPlayer->m_vecAbsVelocity( ) };
	const auto backupDuckAmount{ entry.m_pPlayer->m_flDuckAmount( ) };

	const auto backupCurtime{ Interfaces::Globals->flCurTime };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };

	const auto state{ entry.m_pPlayer->m_pAnimState( ) };
	if ( !state )
		return;

	if ( entry.m_optPreviousData.has_value( ) ) {
		memcpy( entry.m_pPlayer->m_AnimationLayers( ), entry.m_optPreviousData->m_pLayers, 0x38 * entry.m_pPlayer->m_iAnimationLayersCount( ) );

		const auto& layer7 = entry.m_optPreviousData->m_pLayers[ 7 ];
		const auto& layer6 = entry.m_optPreviousData->m_pLayers[ 6 ];
		const auto& layer12 = entry.m_optPreviousData->m_pLayers[ 12 ];

		// these all sync 100% of the time
		state->flFeetCycle == layer6.flCycle;
		state->flMoveWeight == layer6.flWeight;
		state->iStrafeSequence == layer7.nSequence;
		state->flAccelerationWeight == layer12.flWeight;
	}
	else {
		if ( current->m_cAnimData.m_iFlags & FL_ONGROUND )
			state->bHitGroundAnimation = false;

		state->flLastUpdateTime = current->m_cAnimData.m_flSimulationTime - Interfaces::Globals->flIntervalPerTick;
	}

	//float yaw{ entry.m_pPlayer->m_angEyeAngles( ).y };

	//yaw = std::remainderf( yaw, 360.f );

	for ( const auto& interpolated : entry.m_pInterpolatedData ) {
		const int ticks{ TIME_TO_TICKS( interpolated.m_flSimulationTime ) };

		Interfaces::Globals->flCurTime = interpolated.m_flSimulationTime;
		Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

		entry.m_pPlayer->m_flDuckAmount( ) = interpolated.m_flDuckAmount;
		entry.m_pPlayer->m_fFlags( ) = interpolated.m_iFlags;
		entry.m_pPlayer->m_vecVelocity( ) = entry.m_pPlayer->m_vecAbsVelocity( ) = interpolated.m_vecVelocity;

		entry.m_pPlayer->m_iEFlags( ) &= ~EFL_DIRTY_ABSVELOCITY;

		state->flLastUpdateTime = Interfaces::Globals->flCurTime - Interfaces::Globals->flIntervalPerTick;
		state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

		// fix jump_fall if we don't interpolate
		//if ( current->m_bLanded.has_value( ) )
		//	state->flDurationInAir = current->m_flOnGroundTime - current->m_cAnimData.m_flSimulationTime;

		for ( int i = 1; i <= entry.m_pPlayer->m_iAnimationLayersCount( ); i++ )
			entry.m_pPlayer->m_AnimationLayers( )[ i ].pOwner = entry.m_pPlayer;

		entry.m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
		entry.m_pPlayer->UpdateClientsideAnimations( );
		entry.m_pPlayer->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;
	}

	entry.m_pPlayer->m_iEFlags( ) = backupEflags;
	entry.m_pPlayer->m_fFlags( ) = backupFlags;
	entry.m_pPlayer->m_vecVelocity( ) = backupVelocity;
	entry.m_pPlayer->m_vecAbsVelocity( ) = backupAbsVelocity;
	entry.m_pPlayer->m_flDuckAmount( ) = backupDuckAmount;

	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;

	current->m_flAbsYaw = state->flAbsYaw;
	memcpy( entry.m_pPlayer->m_AnimationLayers( ), current->m_cAnimData.m_pLayers, 0x38 * entry.m_pPlayer->m_iAnimationLayersCount( ) );
	entry.m_pPlayer->SetAbsAngles( { 0.f, state->flAbsYaw, 0.f } );

	SetupBonesFixed( entry.m_pPlayer, current->m_pMatrix, 0x0FFF00, //BONE_USED_BY_ANYTHING | BONE_ALWAYS_SETUP, from what i can see in skeet dump, this is all they do... onetap is just bone used by anyhting
		current->m_cAnimData.m_flSimulationTime, ( INVALIDATEBONECACHE | SETUPBONESFRAME | NULLIK | OCCLUSIONINTERP ) );

	const auto mdlData{ entry.m_pPlayer->m_pStudioHdr( ) };
	if ( !mdlData
		|| !mdlData->pStudioHdr )
		return;

	current->m_iBonesCount = mdlData->pStudioHdr->nBones;

	if ( current->m_iBonesCount > 256 )
		current->m_iBonesCount = 256;
}

bool CAnimationSys::SetupBonesFixed( CBasePlayer* const player, matrix3x4_t bones[ 256 ], const int mask, const float time, const int flags ) {
	struct backup_t {
		__forceinline backup_t( CBasePlayer* const player )
			: m_cur_time{ Interfaces::Globals->flCurTime },
			m_frame_time{ Interfaces::Globals->flFrameTime },
			m_frame_count{ Interfaces::Globals->iFrameCount },
			m_occlusion_frame{ player->m_iOcclusionFrame( ) },
			m_ent_client_flags{ player->m_iEntClientFlags( ) },
			m_ik_context{ player->m_IkContext( ) }, m_effects{ player->m_fEffects( ) },
			m_occlusion_flags{ player->m_iOcclusionFlags( ) } {}

		float					m_cur_time{ }, m_frame_time{ };
		int						m_frame_count{ }, m_occlusion_frame{ };

		std::uint8_t			m_ent_client_flags{ };
		ik_context_t* m_ik_context{ };

		int				m_effects{ };
		std::uint32_t m_occlusion_flags{ };
	} backup{ player };

	ctx.m_bForceBoneMask = flags & NULLIK;

	const auto ticks = TIME_TO_TICKS( time );
	Interfaces::Globals->flCurTime = time;
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;
	Interfaces::Globals->iFrameCount = ticks;

	if ( flags & INVALIDATEBONECACHE ) {
		player->g_iModelBoneCounter( ) = 0ul;
		player->m_flLastSetupBonesTime( ) = std::numeric_limits< float >::lowest( );

		auto& boneAccessor = player->m_BoneAccessor( );
		boneAccessor.nWritableBones = boneAccessor.nReadableBones = 0;
	}

	if ( flags & SETUPBONESFRAME )
		player->m_iLastSetupBonesFrame( ) = 0;

	if ( flags & NULLIK ) {
		player->m_IkContext( ) = nullptr;
		player->m_iEntClientFlags( ) |= 2u;
	}

	if ( flags & OCCLUSIONINTERP ) {
		player->m_fEffects( ) |= 8u;

		// C_CSPlayer::ReevauluateAnimLOD and C_CSPlayer::AccumulateLayers
		player->m_iOcclusionFlags( ) &= ~0xau;
		player->m_iOcclusionFrame( ) = 0;
	}

	ctx.m_bSetupBones = true;
	const auto ret = player->SetupBones( bones, 256, mask, time );
	ctx.m_bSetupBones = false;

	if ( flags & NULLIK ) {
		player->m_IkContext( ) = backup.m_ik_context;
		player->m_iEntClientFlags( ) = backup.m_ent_client_flags;
	}

	if ( flags & OCCLUSIONINTERP ) {
		player->m_fEffects( ) = backup.m_effects;
		player->m_iEntClientFlags( ) = backup.m_occlusion_flags;
		player->m_iOcclusionFrame( ) = backup.m_occlusion_frame;
	}

	Interfaces::Globals->flCurTime = backup.m_cur_time;
	Interfaces::Globals->flFrameTime = backup.m_frame_time;
	Interfaces::Globals->iFrameCount = backup.m_frame_count;

	return ret;
}