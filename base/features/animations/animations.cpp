#include "animation.h"
#include "../../utils/performance_monitor.h"
#include "../../utils/threading/thread_pool.hpp"
#include "../../core/hooks.h"

// https://www.youtube.com/watch?v=a3Z7zEc7AXQ
void CAnimationSys::RunAnimationSystem( ) {
	static const auto once = [ ]( )
	{
		const auto fn{ reinterpret_cast< int( _cdecl* )( ) >(
			GetProcAddress( GetModuleHandle( _( "tier0.dll" ) ), _( "AllocateThreadID" ) )
			) };

		std::counting_semaphore<> sem{ 0u };

		for ( std::size_t i{ }; i < std::thread::hardware_concurrency( ); ++i )
			sdk::g_thread_pool->enqueue(
				[ ]( decltype( fn ) fn, std::counting_semaphore<>& sem )
				{
					sem.acquire( );
					fn( );
				}, fn, std::ref( sem )
					);

		for ( std::size_t i{ }; i < std::thread::hardware_concurrency( ); ++i )
			sem.release( );

		sdk::g_thread_pool->wait( );

		return true;
	}( );

	const auto extraTicks{ **( int** )Offsets::Sigs.numticks - **( int** )Offsets::Sigs.host_currentframetick };
	if ( extraTicks )
		return;

	const auto serverCurtime{ TICKS_TO_TIME( Interfaces::ClientState->iServerTick + 2 + TIME_TO_TICKS( ctx.m_flRealOutLatency ) + extraTicks ) };

	const auto flDeadtime{ static_cast< int >( serverCurtime - Offsets::Cvars.sv_maxunlag->GetFloat( ) ) };

	{
		auto updatedPlayers{ new std::vector<PlayerEntry*> };
		CScopedPerformanceMonitor as{ &ctx.m_iAnimsysPerfTimer };

		for ( int i{ 1 }; i <= 64; ++i ) {
			const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
			if ( !player || !player->IsPlayer( ) || player == ctx.m_pLocal )
				continue;

			auto& entry{ m_arrEntries.at( i - 1 ) };

			if ( entry.m_pPlayer != player )
				entry.OnPlayerChange( player );

			if ( player->IsDead( ) )
				continue;

			if ( player->IsDormant( ) ) {
				entry.OutOfDormancy( );
				continue;
			}

			const auto state{ player->m_pAnimState( ) };
			if ( !state )
				continue;

			if ( entry.m_optPreviousData.has_value( )
				&& entry.m_optPreviousData->m_pLayers[ 11 ].flPlaybackRate == player->m_AnimationLayers( )[ 11 ].flPlaybackRate
				&& entry.m_optPreviousData->m_pLayers[ 11 ].flCycle == player->m_AnimationLayers( )[ 11 ].flCycle ) {
				player->m_flSimulationTime( ) = entry.m_optPreviousData->m_flSimulationTime;
				player->m_flOldSimulationTime( ) = entry.m_optPreviousData->m_flOldSimulationTime;
				continue;
			}

			if ( player->m_flSpawnTime( ) != entry.m_flSpawnTime ) {
				state->Reset( );
				entry.OnNewRound( );
			}

			entry.m_pRecords.erase(
				std::remove_if(
					entry.m_pRecords.begin( ), entry.m_pRecords.end( ),
					[ & ]( const std::shared_ptr< LagRecord_t >& lag_record ) -> bool {
						return lag_record->m_cAnimData.m_flSimulationTime < flDeadtime;
					}
				),
				entry.m_pRecords.end( )
						);

			// NOW we add record
			const auto current{ std::make_shared< LagRecord_t >( player ) };

			current->FinalAdjustments( player, entry.m_optPreviousData );

			AnimatePlayer( current.get( ), entry );


			entry.m_optPreviousData = current->m_cAnimData;

			// if record simtime is below deadtime it means there are no records meaning a record will be added. this should use extrapolation, though most of the time the player is airstuck
			const auto noRecords{ entry.m_pRecords.size( )
				&& std::max( player->m_flSimulationTime( ), entry.m_pRecords.back( )->m_cAnimData.m_flSimulationTime )
				+ ctx.m_flRealOutLatency < flDeadtime };

			entry.m_bRecordAdded = ( ( !entry.m_pRecords.size( )
				|| player->m_flSimulationTime( ) > entry.m_pRecords.back( )->m_cAnimData.m_flSimulationTime
				/* || noRecords*/ )
				&& !player->IsTeammate( ) );

			if ( entry.m_bRecordAdded ) {
				entry.m_bBrokeLC = !entry.m_pRecords.empty( ) && ( player->m_vecOrigin( ) - entry.m_pRecords.back( )->m_cAnimData.m_vecOrigin ).LengthSqr( ) > 4096.f;
				if ( entry.m_bBrokeLC )
					entry.m_pRecords.clear( );

				current->m_bBrokeLC = entry.m_bBrokeLC;

				entry.m_pRecords.push_back( current );
			}

			entry.m_iLastRecievedTick = Interfaces::Globals->iTickCount;
			updatedPlayers->push_back( &entry );
		}

		for ( auto& entry : *updatedPlayers ) {
			sdk::g_thread_pool->enqueue( [ ]( PlayerEntry* entry ) {

				std::memcpy( entry->m_pPlayer->m_AnimationLayers( ), entry->m_optPreviousData->m_pLayers, 0x38 * 13 );

				Features::AnimSys.SetupBonesRebuilt( entry->m_pPlayer, entry->m_matMatrix,
					BONE_USED_BY_SERVER, entry->m_optPreviousData->m_flSimulationTime, true );

				if ( entry->m_bRecordAdded ) {
					const auto& record{ entry->m_pRecords.back( ) };
					std::memcpy( record->m_pMatrix, entry->m_matMatrix, entry->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4a_t ) );
				}

				std::memcpy( entry->m_pPlayer->m_CachedBoneData( ).Base( ), entry->m_matMatrix, entry->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4a_t ) );

			}, std::ref( entry ) );

			sdk::g_thread_pool->wait( );
		}

		delete updatedPlayers;
	}
}

void CAnimationSys::AnimatePlayer( LagRecord_t* current, PlayerEntry& entry ) {
	entry.m_bCanExtrapolate = entry.m_iLastNewCmds == current->m_iNewCmds;
	entry.m_iLastNewCmds = current->m_iNewCmds;

	//static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Offsets::Sigs.LookupBone ) };
	//const auto boneIndex{ lookupBone( entry.m_pPlayer, _( "lean_root" ) ) };
	//entry.m_pPlayer->m_pStudioHdr( )->vecBoneFlags[ boneIndex ] = 0;

	const auto backupState{ *entry.m_pPlayer->m_pAnimState( ) };

	entry.m_pPlayer->m_iEFlags( ) |= EFL_DIRTY_ABSTRANSFORM;
	entry.m_pPlayer->SetAbsOrigin( current->m_cAnimData.m_vecOrigin );

	/*const auto isShooting{ current->m_cAnimData.m_pWeapon
		&& current->m_cAnimData.m_flSimulationTime >= current->m_cAnimData.m_pWeapon->m_fLastShotTime( ) 
		&& current->m_cAnimData.m_pWeapon->m_fLastShotTime( ) > entry.m_pPlayer->m_flOldSimulationTime( ) };*/

	current->m_iNewCmds = std::clamp( current->m_iNewCmds, 1, 64 );

	// used in SetUpMovement rebuild
	const auto backupFlash{ entry.m_pPlayer->m_flNextAttack( ) };
	if ( current->m_bFixJumpFall )
		entry.m_pPlayer->m_flNextAttack( ) = current->m_flLeftGroundTime;
	else
		entry.m_pPlayer->m_flNextAttack( ) = 0.f;

	if ( Config::Get<bool>( Vars.RagebotResolver )
		//&& !entry.m_bBot
		&& !ctx.m_pLocal->IsDead( ) ) {
		Rezik( entry.m_pPlayer, current, entry.m_optPreviousData );
	}

	UpdateSide( entry, current );

	entry.m_pPlayer->m_flNextAttack( ) = backupFlash;

	entry.m_vecUpdatedOrigin = entry.m_pPlayer->GetAbsOrigin( );
}

void CAnimationSys::UpdateSide( PlayerEntry& entry, LagRecord_t* current ) {
	const auto backupCurtime{ Interfaces::Globals->flCurTime };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };
	const auto backupHLTV{ Interfaces::ClientState->bIsHLTV };

	Interfaces::ClientState->bIsHLTV = true;
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

	InterpolateFromLastData( entry.m_pPlayer, current, entry.m_optPreviousData );

	Interfaces::ClientState->bIsHLTV = backupHLTV;

	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;
}

void CAnimationSys::InterpolateFromLastData( CBasePlayer* player, LagRecord_t* current, std::optional < AnimData_t >& from ) {
	const auto state{ player->m_pAnimState( ) };
	if ( !from.has_value( ) ) {
		std::memcpy( player->m_AnimationLayers( ), current->m_cAnimData.m_pLayers, 0x38 * 13 );

		if ( current->m_cAnimData.m_iFlags & FL_ONGROUND )
			state->m_bLanding = false;

		state->flDurationInAir = 0.f;
		state->flLastUpdateTime = current->m_cAnimData.m_flSimulationTime - ( Interfaces::Globals->flIntervalPerTick * 2 );

		const auto& layer6{ current->m_cAnimData.m_pLayers[ 6 ] };
		const auto& layer7{ current->m_cAnimData.m_pLayers[ 7 ] };
		const auto& layer12{ current->m_cAnimData.m_pLayers[ 12 ] };

		state->flFeetCycle = layer6.flCycle;
		if ( current->m_cAnimData.m_iFlags & FL_ONGROUND
			&& !current->m_cAnimData.m_pLayers[ 5 ].flWeight 
			&& layer6.flWeight > 0 && layer6.flWeight < 1 )
			state->flMoveWeight = layer6.flWeight;

		state->iStrafeSequence = layer7.nSequence;
		state->flStrafeWeight = layer7.flWeight;
		state->flStrafeCycle = layer7.flCycle;

		state->flAccelerationWeight = layer12.flWeight;

		Interfaces::Globals->flCurTime = state->flLastUpdateTime + Interfaces::Globals->flIntervalPerTick;

		/*if ( current->m_bLanded.has_value( ) )
			state->flDurationInAir = std::max( Interfaces::Globals->flCurTime - Interfaces::Globals->flIntervalPerTick - current->m_flOnGroundTime, 0.f );*/

		if ( current->m_cAnimData.m_pLayers[ 6 ].flPlaybackRate != 0.f
			&& current->m_cAnimData.m_vecVelocity.Length( ) < 1.f )
			current->m_cAnimData.m_vecVelocity = { 1.1f, 0.f, 0.f };

		player->m_vecAbsVelocity( ) = current->m_cAnimData.m_vecVelocity;

		player->m_iEFlags( ) &= ~EFL_DIRTY_ABSVELOCITY;

		state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
		player->UpdateClientsideAnimations( );
		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

		return;
	}

	const auto& layer6{ from->m_pLayers[ 6 ] };
	const auto& layer7{ from->m_pLayers[ 7 ] };
	const auto& layer12{ from->m_pLayers[ 12 ] };

	state->flFeetCycle = layer6.flCycle;
	if ( from->m_iFlags & FL_ONGROUND && current->m_cAnimData.m_iFlags & FL_ONGROUND
		&& !current->m_cAnimData.m_pLayers[ 5 ].flWeight
		&& !from->m_pLayers[ 5 ].flWeight
		&& layer6.flWeight > 0 && layer6.flWeight < 1 )
		state->flMoveWeight = layer6.flWeight;

	state->iStrafeSequence = layer7.nSequence;
	state->flStrafeWeight = layer7.flWeight;
	state->flStrafeCycle = layer7.flCycle;

	state->flAccelerationWeight = layer12.flWeight;

	std::memcpy( player->m_AnimationLayers( ), from->m_pLayers, 0x38 * 13 );

	/* interp */
	const auto& to{ current->m_cAnimData };

	const auto manualStandVel{ current->m_cAnimData.m_pLayers[ 6 ].flPlaybackRate != 0.f
		&& current->m_cAnimData.m_vecVelocity.Length( ) <= 1.f };

	const auto duckAmountDelta{ to.m_flDuckAmount - from->m_flDuckAmount };
	const auto velocityDelta{ to.m_vecVelocity - from->m_vecVelocity };

	bool landed{ };

	Interfaces::Globals->flCurTime = to.m_flSimulationTime - TICKS_TO_TIME( current->m_iNewCmds );

	for ( auto i{ 1 }; i <= current->m_iNewCmds; ++i ) {
		const auto lerp{ i / static_cast< float >( current->m_iNewCmds ) };

		player->m_flDuckAmount( ) = from->m_flDuckAmount + duckAmountDelta * lerp;

		if ( manualStandVel )
			player->m_vecAbsVelocity( ) = { ( i & 1 ) ? 1.1f : -1.1f, 0.f, 0.f };
		else
			player->m_vecAbsVelocity( ) = ( from->m_vecVelocity + ( velocityDelta * lerp ) );

		player->m_iEFlags( ) &= ~EFL_DIRTY_ABSVELOCITY;

		if ( current->m_bFixJumpFall ) {
			player->m_fFlags( ) &= ~FL_ONGROUND;
			if ( !landed ) {
				if ( TIME_TO_TICKS( Interfaces::Globals->flCurTime ) >= TIME_TO_TICKS( current->m_flLeftGroundTime ) ) {
					player->m_fFlags( ) |= FL_ONGROUND;
					landed = true;
				}
			}
		}

		state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
		player->UpdateClientsideAnimations( );
		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

		Interfaces::Globals->flCurTime += Interfaces::Globals->flIntervalPerTick;
	}
}

void CAnimationSys::Rezik( CBasePlayer* player, LagRecord_t* current, std::optional < AnimData_t >& from ) {
	if ( !from.has_value( ) )
		return;

	// perfect resolver
	if ( from->m_flLowerBodyYawTarget != current->m_cAnimData.m_flLowerBodyYawTarget 
		/* || current->m_cAnimData.m_vecVelocity.Length( ) > 0.1f */ ) {
		current->m_bResolverThisTick = true;
		player->m_angEyeAngles( ).y = Math::NormalizeEyeAngles( current->m_cAnimData.m_flLowerBodyYawTarget );
	}
	else {
		//player->m_angEyeAngles( ).y += 180.f;
	}
}

bool CAnimationSys::SetupBonesRebuilt( CBasePlayer* const player, matrix3x4a_t* bones, const int boneMask, const float time, const bool clampbonesinbbox ) {
	//player->SetupBones( bones, 256, boneMask, time );
	//return true;

	auto hdr{ player->m_pStudioHdr( ) };
	if ( !hdr )
		return false;

	auto& boneAccessor{ player->m_BoneAccessor( ) };
	const auto backupBones{ boneAccessor.matBones };
	const auto backupOcclusionFrame{ player->m_iOcclusionFrame( ) };
	const auto backupOcclusionFlags{ player->m_iOcclusionFlags( ) };
	const auto backupEffects{ player->m_fEffects( ) };

	boneAccessor.matBones = bones;

	player->m_fEffects( ) |= EF_NOINTERP;
	player->m_iOcclusionFlags( ) &= ~0xau;
	player->m_iOcclusionFrame( ) = 0;

	Vector pos[ 256 ]{ };
	alignas( 16 ) Quaternion q[ 256 ]{ };
	player->StandardBlendingRules( hdr, pos, q, time, boneMask );

	uint8_t boneComputed[ 256 ]{ };
	std::memset( boneComputed, 0, 256 );
	matrix3x4_t parentTransform{ };
	parentTransform.SetAngles( player->GetAbsAngles( ).y, 0.f, 0.f );
	parentTransform.SetOrigin( player->GetAbsOrigin( ) );
	player->BuildTransformations( hdr, pos, q, parentTransform, boneMask, boneComputed );

	boneAccessor.matBones = backupBones;
	player->m_fEffects( ) = backupEffects;
	player->m_iOcclusionFlags( ) = backupOcclusionFlags;
	player->m_iOcclusionFrame( ) = backupOcclusionFrame;
	return true;
}