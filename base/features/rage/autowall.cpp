#include "autowall.h"

PenetrationData CAutowall::FireBullet( CBasePlayer* const shooter, CBasePlayer* const target, 
	const CCSWeaponData* const weaponData, const bool isTaser, Vector src, const Vector& dst, bool penetrate ) {
	const auto pen_modifier = std::max( ( 3.f / weaponData->flPenetration ) * 1.25f, 0.f );

	PenetrationData data{ };
	float maxDistance{ weaponData->flRange };
	float curDistance{ };

	data.dmg = static_cast< float >( weaponData->iDamage );

	CGameTrace trace{ };
	CTraceFilterSkipTwoEntities traceFilter{ shooter };
	//CTraceFilter traceFilter{ shooter };
	CBasePlayer* lastHitPlayer{ };

	const auto dir{ ( dst - src ).Normalized( ) };

	while ( data.dmg > 0.f ) {
		// calculate max bullet range
		maxDistance -= curDistance;

		// create end point of bullet
		Vector vecEnd{ src + dir * maxDistance };

		traceFilter.pSkip2 = lastHitPlayer;

		// AMD cpus cant do this on a thread bruh
		Interfaces::EngineTrace->TraceRay(
			{ src, vecEnd }, MASK_SHOT_PLAYER,
			reinterpret_cast< ITraceFilter* >( &traceFilter ), &trace
		);

		if ( target && traceFilter.ShouldHitEntity( target, MASK_SHOT_PLAYER ) )
			ClipTraceToPlayer( vecEnd + ( dir * 40.f ), src, &trace, target );

		// we didn't hit anything
		if ( trace.flFraction == 1.f )
			break;

		// calculate the damage based on the distance the bullet traveled.
		curDistance += trace.flFraction * maxDistance;
		data.dmg *= std::powf( weaponData->flRangeModifier, ( curDistance / 500.f ) );

		if ( curDistance > 3000.f && weaponData->flPenetration > 0.f )
			break;

		const auto hitPlayer = static_cast< CBasePlayer* >( trace.pHitEntity );
		if ( hitPlayer
			&& hitPlayer == target ) {
			if ( ( trace.iHitGroup >= HITGROUP_HEAD && trace.iHitGroup <= HITGROUP_RIGHTLEG ) || trace.iHitGroup == HITGROUP_GEAR ) {
				data.target = hitPlayer;
				data.hitbox = trace.iHitbox;
				data.hitgroup = trace.iHitGroup;

				if ( isTaser )
					data.hitgroup = 0;

				ScaleDamage( hitPlayer, data.dmg, weaponData->flArmorRatio, data.hitgroup, weaponData->flHeadShotMultiplier );

				return data;
			}
			lastHitPlayer = hitPlayer->IsPlayer( ) ? hitPlayer : nullptr;
		}
		else
			lastHitPlayer = nullptr;

		// ghetto window fix
		if ( !penetrate && !( trace.iContents & CONTENTS_WINDOW ) )
			break;

		const auto enterSurfaceData{ Interfaces::PhysicsProps->GetSurfaceData( trace.surface.nSurfaceProps ) };
		if ( isTaser 
			|| !enterSurfaceData 
			|| enterSurfaceData->game.flPenetrationModifier < 0.1f
			|| HandleBulletPenetration( shooter, weaponData, trace, src, dir, data.penetrationCount, data.dmg, pen_modifier ) )
			break;
	}

	return { };
}

void CAutowall::ScaleDamage( CBasePlayer* player, float& damage, float ArmourRatio, int hitgroup, float headshotMultiplier ) {
	const auto hasHeavyArmor = player->m_bHasHeavyArmor( );

	switch ( hitgroup ) {
	case 1:
		damage *= headshotMultiplier;

		if ( hasHeavyArmor )
			damage *= 0.5f;

		break;
	case 3: damage *= 1.25f; break;
	case 6:
	case 7:
		damage *= 0.75f;

		break;
	}

	const auto armor_value = player->m_ArmorValue( );
	if ( !armor_value
		|| hitgroup < 0
		|| hitgroup > 5
		|| ( hitgroup == 1 && !player->m_bHasHelmet( ) ) )
		return;

	auto heavy_ratio = 1.f, bonus_ratio = 0.5f, ratio = ArmourRatio * 0.5f;

	if ( hasHeavyArmor ) {
		ratio *= 0.2f;
		heavy_ratio = 0.25f;
		bonus_ratio = 0.33f;
	}

	auto dmg_to_hp = damage * ratio;

	if ( ( ( damage - dmg_to_hp ) * ( bonus_ratio * heavy_ratio ) ) > armor_value )
		damage -= armor_value / bonus_ratio;
	else
		damage = dmg_to_hp;
}

void CAutowall::ClipTraceToPlayer( Vector dst, Vector src, CGameTrace* oldtrace, CBasePlayer* ent ) {
	const auto pos{ ent->m_vecOrigin( ) + ( ent->m_vecMins( ) + ent->m_vecMaxs( ) ) * 0.5f };
	const auto to{ pos - src };

	auto dir{ src - dst };
	const auto len{ dir.NormalizeInPlace( ) };
	const auto range_along{ dir.DotProduct( to ) };

	const auto range{ 
		range_along < 0.f ? -( to ).Length( )
		: range_along > len ? -( pos - dst ).Length( )
		: ( pos - ( src + dir * range_along ) ).Length( ) };

	if ( range > 60.f )
		return;

	CGameTrace new_trace{ };

	Ray_t Ray{ src, dst };

	Interfaces::EngineTrace->ClipRayToEntity( Ray, MASK_SHOT_PLAYER, ent, &new_trace );

	if ( new_trace.flFraction > oldtrace->flFraction )
		return;

	*oldtrace = new_trace;
}

/*
if ( ( nCurrentContents & MASK_SHOT_HULL ) == 0 || ( nCurrentContents & CONTENTS_HITBOX ) != 0 && nCurrentContents != nStartContents ) {
			Interfaces::EngineTrace->TraceRay( { end, vecTrEnd }, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr, &trExit );

			if ( trExit.bStartSolid && trExit.surface.uFlags < 0 ) {
				// do another trace, but skip the player to get the actual exit surface
				auto filter = CTraceFilter( trExit.pHitEntity, TRACE_EVERYTHING );//COLLISION_GROUP_NONE

				Interfaces::EngineTrace->TraceRay( { end, start }, MASK_SHOT_HULL, &filter, &trExit );

				if ( ( 1.f > trExit.flFraction || trExit.bAllSolid )
					&& !trExit.bStartSolid )
					return true;
			}
			else if ( 1.f <= trExit.flFraction && !trExit.bAllSolid || trExit.bStartSolid ) {
				if ( trEnter.pHitEntity
					&& !( trEnter.pHitEntity == ctx.m_pLocal )
					&& trEnter.pHitEntity->IsBreakable( ) ) {
					trExit.surface.szName = trEnter.surface.szName;
					trExit.vecEnd = start + dir;

					//*( _WORD* )( exit_trace + 64 ) = *( _WORD* )( enter_trace + 64 );
					return true;
				}
			}
			else {
				if ( trExit.surface.uFlags >= 0 )
					goto LABEL_25;

				if ( trExit.pHitEntity->IsBreakable( )
					&& trEnter.pHitEntity->IsBreakable( ) )
					return true;

				if ( trExit.surface.uFlags >> 7 ) {
				LABEL_25:
					if ( trExit.plane.vecNormal.DotProduct( dir ) <= 1.f )
						return true;
				}
			}
		}*/
bool CAutowall::TraceToExit( const Vector& src, const Vector& dir, const CGameTrace& enter_trace, CGameTrace& exit_trace ) {
	float dist{ };

	int32_t firstContents = 0;

	constexpr auto k_step_size = 4.f;
	constexpr auto k_max_dist = 90.f;

	while ( dist <= k_max_dist ) {
		dist += k_step_size;

		const auto out = src + ( dir * dist );

		const auto curContents = Interfaces::EngineTrace->GetPointContents( out, MASK_SHOT_PLAYER );

		if ( !firstContents )
			firstContents = curContents;

		if ( curContents & MASK_SHOT_HULL
			&& ( !( curContents & CONTENTS_HITBOX ) || curContents == firstContents ) )
			continue;

		Interfaces::EngineTrace->TraceRay( { out, out - dir * k_step_size }, MASK_SHOT_PLAYER, nullptr, &exit_trace );

		if ( exit_trace.bStartSolid
			&& exit_trace.surface.uFlags & SURF_HITBOX ) {
			CTraceFilter traceFilter{ exit_trace.pHitEntity };

			Interfaces::EngineTrace->TraceRay(
				{ out, src }, MASK_SHOT_HULL,
				reinterpret_cast< ITraceFilter* >( &traceFilter ), &exit_trace
			);

			if ( exit_trace.DidHit( )
				&& !exit_trace.bStartSolid )
				return true;

			continue;
		}

		if ( !exit_trace.DidHit( )
			|| exit_trace.bStartSolid ) {
			if ( enter_trace.pHitEntity
				&& enter_trace.pHitEntity->Index( )
				&& enter_trace.pHitEntity->IsBreakable( ) ) {
				exit_trace = enter_trace;
				exit_trace.vecEnd = src + dir;

				return true;
			}

			continue;
		}

		if ( exit_trace.surface.uFlags & SURF_NODRAW ) {
			if ( exit_trace.pHitEntity->IsBreakable( )
				&& enter_trace.pHitEntity->IsBreakable( ) )
				return true;

			if ( !( enter_trace.surface.uFlags & SURF_NODRAW ) )
				continue;
		}

		if ( exit_trace.plane.vecNormal.DotProduct( dir ) <= 1.f )
			return true;
	}

	return false;
}

bool CAutowall::HandleBulletPenetration( CBasePlayer* const shooter, const CCSWeaponData* const weaponData, 
	const CGameTrace& enterTrace, Vector& src, const Vector& dir, int& penCount, float& curDmg, 
	const float pen_modifier ) {
	if ( penCount <= 0
		|| weaponData->flPenetration <= 0.f )
		return true;

	CGameTrace exitTrace{ };

	if ( !TraceToExit( enterTrace.vecEnd, dir, enterTrace, exitTrace )
		&& !( Interfaces::EngineTrace->GetPointContents( enterTrace.vecEnd, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) )
		return true;

	auto finalDmgMod = 0.16f;
	float combinedPenMod{ };

	const auto exitSurfaceData = Interfaces::PhysicsProps->GetSurfaceData( exitTrace.surface.nSurfaceProps );
	const auto enterSurfaceData = Interfaces::PhysicsProps->GetSurfaceData( enterTrace.surface.nSurfaceProps );

	if ( enterSurfaceData->game.hMaterial == 'G'
		|| enterSurfaceData->game.hMaterial == 'Y' ) {
		finalDmgMod = 0.05f;
		combinedPenMod = 3.f;
	}
	else if ( enterTrace.iContents & CONTENTS_GRATE
		|| enterTrace.surface.uFlags & SURF_NODRAW ) {
		finalDmgMod = 0.16f;
		combinedPenMod = 1.f;
	}
	else if ( enterTrace.pHitEntity
		&& Offsets::Cvars.ff_damage_reduction_bullets->GetFloat( ) == 0.f
		&& enterSurfaceData->game.hMaterial == 'F'
		&& static_cast<CBasePlayer*>( enterTrace.pHitEntity )->IsTeammate( shooter ) ) {
		const auto dmg_bullet_pen = Offsets::Cvars.ff_damage_bullet_penetration->GetFloat( );
		if ( dmg_bullet_pen == 0.f )
			return true;

		combinedPenMod = dmg_bullet_pen;
		finalDmgMod = 0.16f;
	}
	else {
		combinedPenMod = (
			enterSurfaceData->game.flPenetrationModifier
			+ exitSurfaceData->game.flPenetrationModifier
			) * 0.5f;

		finalDmgMod = 0.16f;
	}

	if ( enterSurfaceData->game.hMaterial == exitSurfaceData->game.hMaterial ) {
		if ( exitSurfaceData->game.hMaterial == 'U'
			|| exitSurfaceData->game.hMaterial == 'W' )
			combinedPenMod = 3.f;
		else if ( exitSurfaceData->game.hMaterial == 'L' )
			combinedPenMod = 2.f;
	}

	const auto modifier = std::max( 1.f / combinedPenMod, 0.f );
	const auto penDist = ( exitTrace.vecEnd - enterTrace.vecEnd ).Length( );

	const auto lostDmg =
		curDmg * finalDmgMod
		+ pen_modifier * ( modifier * 3.f )
		+ ( ( penDist * penDist ) * modifier ) / 24.f;

	if ( lostDmg > curDmg )
		return true;

	if ( lostDmg > 0.f )
		curDmg -= lostDmg;

	if ( curDmg < 1.f )
		return true;

	--penCount;

	src = exitTrace.vecEnd;

	return false;
}

bool CAutowall::CanPenetrate( ) {
	if ( ctx.m_pLocal->IsDead( ) )
		return false;

	if ( !ctx.m_pWeapon || ctx.m_pWeapon->IsKnife( ) || 
		ctx.m_pWeapon->IsGrenade( ) || !ctx.m_pWeaponData )
		return false;

	Vector direction;
	Math::AngleVectors( ctx.m_angOriginalViewangles, &direction );

	CGameTrace trace;
	CTraceFilter filter{ ctx.m_pLocal };

	Interfaces::EngineTrace->TraceRay(
		{ ctx.m_vecEyePos, ctx.m_vecEyePos + direction * ctx.m_pWeaponData->flRange }, MASK_SHOT_HULL,
		reinterpret_cast< ITraceFilter* >( &filter ), &trace
	);

	if ( trace.flFraction == 1.0f )
		return false;

	int penCount{ 1 };
	float dmg = static_cast< float >( ctx.m_pWeaponData->iDamage );
	const auto penModifier = std::max( ( 3.f / ctx.m_pWeaponData->flPenetration ) * 1.25f, 0.f );

	const bool e = !HandleBulletPenetration( ctx.m_pLocal, ctx.m_pWeaponData, trace, ctx.m_vecEyePos, direction, penCount, dmg, penModifier );
	ctx.m_iPenetrationDamage = static_cast<int>( dmg ) == ctx.m_pWeaponData->iDamage ? 0 : dmg;

	return e;
}

PenetrationData CAutowall::FireEmulated( CBasePlayer* const shooter, CBasePlayer* const target, Vector src, const Vector& dst ) {
	static const auto wpn_data = [ ]( ) {
		CCSWeaponData wpn_data{ };

		wpn_data.iDamage = 115;
		wpn_data.flRange = 8192.f;
		wpn_data.flPenetration = 2.5f;
		wpn_data.flRangeModifier = 0.99f;
		wpn_data.flArmorRatio = 1.95f;

		return wpn_data;
	}( );

	const auto pen_modifier = std::max( ( 3.f / wpn_data.flPenetration ) * 1.25f, 0.f );

	float cur_dist{ };

	PenetrationData data{ };

	data.penetrationCount = 4;

	auto cur_dmg = static_cast< float >( wpn_data.iDamage );

	auto dir = dst - src;

	const auto max_dist = dir.NormalizeInPlace( );

	CGameTrace trace{ };
	CTraceFilterSkipTwoEntities traceFilter{ shooter };

	while ( cur_dmg > 0.f ) {
		const auto dist_remaining = wpn_data.flRange - cur_dist;

		const auto cur_dst = src + dir * dist_remaining;

		traceFilter.pSkip2 = trace.pHitEntity && trace.pHitEntity->IsPlayer( ) ? trace.pHitEntity : nullptr;

		Interfaces::EngineTrace->TraceRay(
			{ src, cur_dst }, MASK_SHOT_PLAYER,
			reinterpret_cast< ITraceFilter* >( &traceFilter ), &trace
		);

		Vector ExEnd{ cur_dst + dir * 40.f };

		if ( target )
			ClipTraceToPlayer( ExEnd, src, &trace, target );

		if ( trace.flFraction == 1.f
			|| ( trace.vecEnd - src ).Length( ) > max_dist )
			break;

		cur_dist += trace.flFraction * dist_remaining;
		cur_dmg *= std::pow( wpn_data.flRangeModifier, cur_dist / 500.f );

		if ( cur_dist > 3000.f
			&& wpn_data.flPenetration > 0.f )
			break;

		if ( trace.pHitEntity ) {
			const auto is_player = trace.pHitEntity->IsPlayer( );
			if ( trace.pHitEntity == target ) {
				data.target = static_cast< CBasePlayer* >( trace.pHitEntity );
				data.hitbox = trace.iHitbox;
				data.hitgroup = trace.iHitGroup;
				data.dmg = static_cast< int >( cur_dmg );

				return data;
			}
		}

		const auto enter_surface = Interfaces::PhysicsProps->GetSurfaceData( trace.surface.nSurfaceProps );
		if ( enter_surface->game.flPenetrationModifier < 0.1f
			|| HandleBulletPenetration( shooter, &wpn_data, trace, src, dir, data.penetrationCount, cur_dmg, pen_modifier ) )
			break;
	}

	return data;
}