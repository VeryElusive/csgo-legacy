#include "animation.h"
#include "../../core/lua/scripting.h"

void Animate( CBasePlayer* player ) {
	const auto backupCurtime{ Interfaces::Globals->flCurTime };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };
	const auto backupHLTV{ Interfaces::ClientState->bIsHLTV };

	Interfaces::ClientState->bIsHLTV = true;
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

	player->m_pAnimState( )->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

	player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
	player->UpdateClientsideAnimations( );
	player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

	Interfaces::ClientState->bIsHLTV = backupHLTV;
	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;
}

float BackPos( CBasePlayer* player ) {
	QAngle shootAngle;

	auto eyePos = player->m_vecOrigin( );
	eyePos.z += player->m_flDuckAmount( ) ? 46.f : 64.f;

	const auto hitboxSet{ ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) ) };
	const auto hitbox{ hitboxSet->GetHitbox( HITBOX_CHEST ) };
	if ( !hitbox )
		return 0;

	const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] ) };

	Math::VectorAngles( point - eyePos, shootAngle );

	return Math::NormalizeEyeAngles( shootAngle.y + 180.f );
}

void CAnimationSys::Resolver( PlayerEntry& entry, LagRecord_t* current ) {
	// ez resolve
	if ( entry.m_pPlayer->m_flLowerBodyYawTarget( ) != entry.m_flLowerBodyYawTarget
		|| current->m_cAnimData.m_pLayers[ 6 ].flPlaybackRate
		|| entry.m_pPlayer->m_flSimulationTime( ) > entry.m_flLowerBodyRealignTimer ) {
		current->m_bLBYUpdate = true;
		current->m_cAnimData.m_bLBYUpdate = true;

		entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_pPlayer->m_flLowerBodyYawTarget( );
		current->m_angEyeAngles.y = entry.m_pPlayer->m_flLowerBodyYawTarget( );


		if ( current->m_cAnimData.m_vecVelocity.Length2D( ) > 0.1f )
			entry.m_flLowerBodyRealignTimer = entry.m_pPlayer->m_flSimulationTime( ) + 0.22f;
		else if ( entry.m_pPlayer->m_flLowerBodyYawTarget( ) != entry.m_flLowerBodyYawTarget )
			entry.m_flLowerBodyRealignTimer = entry.m_pPlayer->m_flSimulationTime( ) + 1.1f;

		entry.m_flLowerBodyYawTarget = entry.m_pPlayer->m_flLowerBodyYawTarget( );
		return;
	}

	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	if ( !entry.m_optPreviousData.has_value( ) )
		return;

	Scripting::DoCallback( FNV1A::HashConst( "resolver" ), entry, current->m_cAnimData );
}