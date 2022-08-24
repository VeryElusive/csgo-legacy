#include "../../core/hooks.h"
#include "../../context.h"
#include "../../features/rage/antiaim.h"
#include "../../features/animations/animation.h"
#include <intrin.h>

void FASTCALL Hooks::hkStandardBlendingRules( CBasePlayer* const ent, const std::uintptr_t edx, CStudioHdr* const mdl_data, int a1, int a2, float a3, int mask ) {
	static auto oStandardBlendingRules = DTR::StandardBlendingRules.GetOriginal<decltype( &hkStandardBlendingRules )>( );
	if ( !ent || !ctx.m_pLocal || !ent->IsPlayer( ) )
		return oStandardBlendingRules( ent, edx, mdl_data, a1, a2, a3, mask );

	if ( ctx.m_bForceBoneMask )
		mask = 0x100;

	static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Offsets::Sigs.LookupBone ) };

	const auto bone_index = lookupBone( ent, _( "lean_root" ) );
	if ( bone_index == -1 )
		return oStandardBlendingRules( ent, edx, mdl_data, a1, a2, a3, mask );

	auto& boneFlags{ mdl_data->vecBoneFlags[ bone_index ] };

	const auto backupBoneFlags = boneFlags;

	boneFlags = 0u;

	oStandardBlendingRules( ent, edx, mdl_data, a1, a2, a3, mask );

	boneFlags = backupBoneFlags;
}

void FASTCALL Hooks::hkDoExtraBonesProcessing( void* ecx, uint32_t ye, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context ) {
	return;
}

bool FASTCALL Hooks::hkShouldSkipAnimFrame( void* ecx, uint32_t ebx ) {
	static auto oShouldSkipAnimFrame{ DTR::ShouldSkipAnimFrame.GetOriginal<decltype( &hkShouldSkipAnimFrame )>( ) };
	if ( ctx.m_bSetupBones )
		return false;

	return oShouldSkipAnimFrame( ecx, ebx );
}

void FASTCALL Hooks::hkClampBonesInBBox( CBasePlayer* ecx, uint32_t ebx, matrix3x4_t* bones, int boneMask ) {
	static auto oClampBonesInBBox{ DTR::ClampBonesInBBox.GetOriginal<decltype( &hkClampBonesInBBox )>( ) };

	if ( ecx == ctx.m_pLocal
		&& !( ctx.m_cLocalData.at( ctx.m_iLastSentCmdNumber % 150 ).PredictedNetvars.m_iFlags & FL_ONGROUND ) )
		return;

	/* guwop fix
	static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Offsets::Sigs.LookupBone ) };
	const auto bone_index = lookupBone( ecx, _( "pelvis" ) );

	const auto oldBonePos = bones[ bone_index ].GetOrigin( );*/

	oClampBonesInBBox( ecx, ebx, bones, boneMask );

	/*const auto newBonePos = bones[ bone_index ].GetOrigin( );

	const auto delta = oldBonePos - newBonePos;
	for ( std::size_t i{ }; i < ecx->m_CachedBoneData( ).Count( ); ++i ) {
		auto& bone = bones[ i ];
		auto& mat = bones[ i ];
		bone[ 0 ][ 3 ] = mat[ 0 ][ 3 ] + delta.x;
		bone[ 1 ][ 3 ] = mat[ 1 ][ 3 ] + delta.y;
		bone[ 2 ][ 3 ] = mat[ 2 ][ 3 ] + delta.z;
	}*/
}

void FASTCALL Hooks::hkPostDataUpdate( CBasePlayer* ecx, uint32_t edx, int32_t updateType ) {
	static auto oPostDataUpdate{ DTR::PostDataUpdate.GetOriginal<decltype( &hkPostDataUpdate )>( ) };

	if ( !ecx
		|| !ecx->IsPlayer( ) )
		return oPostDataUpdate( ecx, edx, updateType );

	const auto backupEffects{ ecx->m_fEffects( ) };

	ecx->m_fEffects( ) |= EF_NOINTERP;
	oPostDataUpdate( ecx, edx, updateType );
	ecx->m_fEffects( ) = backupEffects;
}

bool FASTCALL Hooks::hkSetupbones( const std::uintptr_t ecx, const std::uintptr_t edx, matrix3x4_t* const bones, 
	int max_bones, int mask, float time ) {
	static auto oSetupbones{ DTR::Setupbones.GetOriginal<decltype( &hkSetupbones )>( ) };
	const auto player = reinterpret_cast< CBasePlayer* >( ecx - sizeof( std::uintptr_t ) );
	if ( player->IsDead( )
		|| !player->IsPlayer( )
		|| ( mask == BONE_USED_BY_ATTACHMENT && player != ctx.m_pLocal ) )// ill let de_game do it for me!
		return oSetupbones( ecx, edx, bones, max_bones, mask, time );

	if ( !ctx.m_bSetupBones ) {
		if ( !bones
			|| max_bones == -1 )
			return true;

		if ( player == ctx.m_pLocal ) {
			std::memcpy(
				bones, ctx.m_matRealLocalBones->Base( ),
				std::min( max_bones, 256 ) * sizeof( matrix3x4_t )
			);
		}
		else {
			std::memcpy(
				bones, player->m_CachedBoneData( ).Base( ),
				std::min( max_bones, 256 ) * sizeof( matrix3x4_t )
			);
		}

		return true;
	}

	return oSetupbones( ecx, edx, bones, max_bones, mask, time );
}
void FASTCALL Hooks::hkUpdateClientsideAnimation( CBasePlayer* ecx, void* edx ) {
	static auto oUpdateClientsideAnimation{ DTR::UpdateClientsideAnimation.GetOriginal<decltype( &hkUpdateClientsideAnimation )>( ) };
	if ( ecx->IsDead( )
		|| !ecx->IsPlayer( )
		|| ecx != ctx.m_pLocal )
		return oUpdateClientsideAnimation( ecx, edx );

	if ( !ctx.m_bUpdatingAnimations )
		return;

	oUpdateClientsideAnimation( ecx, edx );
}

QAngle* FASTCALL Hooks::hkGetEyeAngles( CBasePlayer* ecx, void* edx ) {
	static auto oGetEyeAngles = DTR::GetEyeAngles.GetOriginal<decltype( &hkGetEyeAngles )>( );

	auto& LocalData{ ctx.m_cLocalData.at( ctx.m_iLastSentCmdNumber % 150 ) };
	if ( ecx != ctx.m_pLocal
		|| LocalData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( )
		|| *reinterpret_cast< std::uintptr_t* >( _AddressOfReturnAddress( ) ) != Offsets::Sigs.ReturnToEyePosAndVectors )
		return oGetEyeAngles( ecx, edx );

	return &LocalData.m_angViewAngles;
}

void FASTCALL Hooks::hkCheckForSequenceChange( void* ecx, int edx, void* hdr, int cur_sequence, bool force_new_sequence, bool interpolate ) {
	// no sequence interpolation over here mate
	// forces the animation queue to clear
	static auto oCheckForSequenceChange = DTR::CheckForSequenceChange.GetOriginal<decltype( &hkCheckForSequenceChange )>( );
	auto* player = reinterpret_cast< CBasePlayer* >( uintptr_t( ecx ) - 4 );

	if ( !ecx || player || !player->IsPlayer( ) )
		return oCheckForSequenceChange( ecx, edx, hdr, cur_sequence, force_new_sequence, interpolate );

	return oCheckForSequenceChange( ecx, edx, hdr, cur_sequence, force_new_sequence, false );
}

void FASTCALL Hooks::hkAccumulateLayers( CBasePlayer* const ecx, const std::uintptr_t edx, int a0, int a1, float a2, int a3 ) {
	static auto oAccumulateLayers = DTR::AccumulateLayers.GetOriginal<decltype( &hkAccumulateLayers )>( );
	if ( !ecx->IsPlayer( ) )
		return oAccumulateLayers( ecx, edx, a0, a1, a2, a3 );

	if ( const auto state = ecx->m_pAnimState( ) ) {
		const auto backup_first_update = state->bFirstUpdate;

		state->bFirstUpdate = true;

		oAccumulateLayers( ecx, edx, a0, a1, a2, a3 );

		state->bFirstUpdate = backup_first_update;

		return;
	}

	oAccumulateLayers( ecx, edx, a0, a1, a2, a3 );
}

void FASTCALL Hooks::hkOnLatchInterpolatedVariables( CBasePlayer* const ecx, const std::uintptr_t edx, const int flags ) {
	static auto oOnLatchInterpolatedVariables = DTR::OnLatchInterpolatedVariables.GetOriginal<decltype( &hkOnLatchInterpolatedVariables )>( );
	if ( !Interfaces::Engine->IsInGame( )
		|| ecx != ctx.m_pLocal )
		return oOnLatchInterpolatedVariables( ecx, edx, flags );

	const auto backupSimTime = ecx->m_flSimulationTime( );

	const auto& localData = ctx.m_cLocalData.at( Interfaces::ClientState->iCommandAck % 150 );
	if ( localData.m_flSpawnTime == ctx.m_pLocal->m_flSpawnTime( )
		&& localData.m_iShiftAmount > 0 )
		ecx->m_flSimulationTime( ) += TICKS_TO_TIME( localData.m_iShiftAmount );

	oOnLatchInterpolatedVariables( ecx, edx, flags );

	ecx->m_flSimulationTime( ) = backupSimTime;
}