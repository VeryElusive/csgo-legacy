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

		entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_pPlayer->m_flLowerBodyYawTarget( );
		current->m_angEyeAngles.y = entry.m_pPlayer->m_flLowerBodyYawTarget( );


		if ( current->m_cAnimData.m_vecVelocity.Length2D( ) > 0.1f )
			entry.m_flLowerBodyRealignTimer = entry.m_pPlayer->m_flSimulationTime( ) + 0.22f;
		else if ( entry.m_pPlayer->m_flLowerBodyYawTarget( ) != entry.m_flLowerBodyYawTarget )
			entry.m_flLowerBodyRealignTimer = entry.m_pPlayer->m_flSimulationTime( ) + 1.1f;

		entry.m_flLowerBodyYawTarget = entry.m_pPlayer->m_flLowerBodyYawTarget( );
		return;
	}

	Scripting::DoCallback( FNV1A::HashConst( "resolver" ), entry.m_iMissedShots, entry.m_flFirstShotTime );
	return;

	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	if ( !entry.m_optPreviousData.has_value( ) )
		return;

	if ( Interfaces::Globals->flRealTime > entry.m_flFirstShotTime + 15.f )
		entry.m_iMissedShots = 0;

	if ( entry.m_iMissedShots 
		&& Interfaces::Globals->flRealTime >= entry.m_flFirstShotTime
		&& Interfaces::Globals->flRealTime < entry.m_flFirstShotTime + 15.f ) {
		switch ( entry.m_iMissedShots % 5 ) {
		case 0:
			entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_flFirstShotEyeYaw;
			break;
		case 1:
			entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_flFirstShotEyeYaw + 180.f;
			break;
		case 2:
			entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_flFirstShotEyeYaw + 90.f;
			break;
		case 3:
			entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_flFirstShotEyeYaw - 45.f;
			break;
		case 4:
			entry.m_pPlayer->m_angEyeAngles( ).y = entry.m_flFirstShotEyeYaw - 45.f;
			break;
		}

		return;
	}

	const auto hitboxSet{ entry.m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( entry.m_pPlayer->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return;

	const auto hitbox{ hitboxSet->GetHitbox( HITBOX_HEAD ) };
	if ( !hitbox )
		return;

	const auto matrix{ new matrix3x4a_t[ 256 ] };

	const auto backupEyeAngles{ entry.m_pPlayer->m_angEyeAngles( ).y };

	float bestDamage{ };
	char bestPos{ };

	static CCSWeaponData maxGun{};
	maxGun.iDamage = 200;
	maxGun.flRangeModifier = 1.0f;
	maxGun.flPenetration = 6.0f;
	maxGun.flArmorRatio = 2.0f;
	maxGun.flRange = 8192.f;

	for ( char i{ 0 }; i <= 3; ++i ) {
		entry.m_pPlayer->m_angEyeAngles( ).y = backupEyeAngles + ( 90.f * i );
		entry.m_pPlayer->m_angEyeAngles( ).y = Math::NormalizeEyeAngles( entry.m_pPlayer->m_angEyeAngles( ).y );

		Animate( entry.m_pPlayer );

		Features::AnimSys.SetupBonesRebuilt( entry.m_pPlayer, matrix,
			BONE_USED_BY_SERVER, entry.m_pPlayer->m_flSimulationTime( ), false );

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, matrix[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, entry.m_pPlayer, &maxGun, false, ctx.m_vecEyePos, point, true ) };

		if ( data.dmg > bestDamage ) {
			bestPos = i;
			bestDamage = data.dmg;
		}
	}

	delete[ ] matrix;

	entry.m_pPlayer->m_angEyeAngles( ).y = backupEyeAngles;

	if ( bestPos ) {
		entry.m_pPlayer->m_angEyeAngles( ).y += ( 90.f * bestPos ) + 180.f;
		entry.m_pPlayer->m_angEyeAngles( ).y = Math::NormalizeEyeAngles( entry.m_pPlayer->m_angEyeAngles( ).y );
		return;
	}
	
	entry.m_pPlayer->m_angEyeAngles( ).y = BackPos( entry.m_pPlayer );
}