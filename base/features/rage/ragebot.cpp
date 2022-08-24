#include "ragebot.h"
#include "exploits.h"
#include "../visuals/visuals.h"
#include "../../utils/threading/threading.h"

void CRageBot::GetTargets( void* i ) {
	auto actualI{ ( int )i };

	auto& entry = Features::AnimSys.m_arrEntries.at( actualI - 1 );

	const auto record{ Features::Ragebot.GetBestLagRecord( entry ) };
	if ( !record )
		return;

	Features::Ragebot.m_pMutex.lock( );
	Features::Ragebot.m_cAimTargets.push_back( { record, entry.m_pPlayer, entry.m_iMissedShots } );
	Features::Ragebot.m_pMutex.unlock( );
}

// TODO: are these vector rotate shit right?
void CRageBot::Main( CUserCmd& cmd ) {
	Reset( );

	if ( !Config::Get<bool>( Vars.RagebotEnable )
		|| !ctx.m_pWeapon || !ctx.m_pWeaponData
		|| ctx.m_pWeapon->m_bReloading( )
		|| ( ctx.m_pWeaponData->nWeaponType >= WEAPONTYPE_C4
			&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_TASER )
		|| ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER )
		return;

	if ( ctx.m_pWeapon->IsKnife( ) ) {
		//feature::knifebot.main( cmd );
		return;
	}
	else if ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER && !Config::Get<bool>( Vars.RagebotZeusbot ) )
		return;

	if ( ctx.m_pWeaponData->nWeaponType != m_iLastWeaponType 
		|| ctx.m_pWeapon->m_iItemDefinitionIndex( ) != m_iLastWeaponIndex
		|| Menu::Opened )
		ParseCfgItems( ctx.m_pWeaponData->nWeaponType );

	SetupHitboxes( );

	if ( m_iHitboxes.size( ) < 1 )
		return;

	for ( auto i{ 1 }; i <= 64; i++ ) {
		auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player || !player->IsPlayer( ) || player == ctx.m_pLocal || player->m_bGunGameImmunity( ) || player->IsTeammate( ) )
			continue;

		const auto fov = Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) );
		if ( fov > RagebotFOV )
			continue;

		Threading::QueueJobRef( GetTargets, (void*)i );
	}

	Threading::FinishQueue( );

	ScanTargets( );
	Fire( cmd );

	m_iHitboxes.clear( );
	m_cAimTargets.clear( );
}

void CRageBot::ScanTargets( ) {
	for ( auto& target : m_cAimTargets ) {
		const auto backup = std::make_unique< LagBackup_t >( target.m_pPlayer );

		if ( !CreatePoints( target, target.m_cPoints ) )
			continue;

		target.m_pRecord->Apply( target.m_pPlayer );

		for ( auto& point : target.m_cPoints )
			ScanPoint( target.m_pPlayer, target.m_pRecord, point );
		
		target.m_cAimPoint = PickPoints( target.m_pPlayer, target.m_cPoints );

		target.m_cPoints.clear( );

		backup->Apply( target.m_pPlayer );
	}
}

std::size_t CRageBot::CalcPointCount( mstudiohitboxset_t* hitboxSet ) {
	std::size_t ret{ };

	for ( const auto& hb : m_iHitboxes ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		if ( Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).enabled && hb == HITBOX_HEAD )
			continue;

		// ignore limbs when moving
		if ( RagebotIgnoreLimbs && ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 1.f
			&& ( hitbox->iGroup == HITGROUP_LEFTLEG || hitbox->iGroup == HITGROUP_RIGHTLEG
				|| hitbox->iGroup == HITGROUP_LEFTARM || hitbox->iGroup == HITGROUP_RIGHTARM ) )
			continue;

		ret++;

		if ( !IsMultiPointEnabled( hb ) )
			continue;

		if ( hitbox->flRadius <= 0.f )
			ret += 2;
		else {
			if ( hb ) {
				if ( hb != 3 ) {
					if ( hb != 2
						&& hb != 6 ) {
						if ( hb == 4
							|| hb == 5 )
							ret++;

						continue;
					}

					if ( hb == 6 ) {
						ret++;
						continue;
					}
				}

				ret += 6;
				continue;
			}

			ret += 4;
		}

	}

	return ret;
}

bool CRageBot::CreatePoints( AimTarget_t& aimTarget, std::vector<AimPoint_t>& aimPoints ) {
	const auto hitboxSet = aimTarget.m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( aimTarget.m_pPlayer->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	const auto& missed = aimTarget.m_iMissedShots;

	aimPoints.reserve( CalcPointCount( hitboxSet ) );

	for ( const auto& hb : m_iHitboxes ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		if ( Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).enabled && hb == HITBOX_HEAD )
			continue;

		// ignore limbs when moving
		if ( RagebotIgnoreLimbs && ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 1.f
			&& ( hitbox->iGroup == HITGROUP_LEFTLEG || hitbox->iGroup == HITGROUP_RIGHTLEG
				|| hitbox->iGroup == HITGROUP_LEFTARM || hitbox->iGroup == HITGROUP_RIGHTARM ) )
			continue;

		int scale = ( hb ? RagebotBodyScale : RagebotHeadScale );

		if ( !RagebotStaticPointscale ) {
			// dynamic multipoints:
			// check the side delta + inac
			scale = 100;

			// this will take us down to a maximum 35 scale
			scale -= std::floor( std::min( Features::EnginePrediction.Inaccuracy + Features::EnginePrediction.Spread, 0.15f ) * 333.3333333333f );

			// now compare side delta
			scale -= std::min( 90 - std::abs( 90 - OffsetDelta( aimTarget.m_pPlayer, aimTarget.m_pRecord ) ), scale );
		}

		float scaleFloat{ scale / 100.f };

		auto& matrix{ aimTarget.m_pRecord->m_cAnimData.m_cAnimSides.at( aimTarget.m_pRecord->m_iResolverSide ).m_pMatrix[ hitbox->iBone ] };

		Vector center{ ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f };
		center = Math::VectorTransform( center, matrix );

		auto& point{ aimPoints.emplace_back( center, hitbox->iGroup ) };

		//ScanPoint( aimTarget.m_pPlayer, aimTarget.m_pRecord, point );
		
		// opt
		if ( IsMultiPointEnabled( hb ) /* && point.m_flDamage > 0*/ )
			Multipoint( center, matrix, aimPoints, hitbox, hitboxSet, scaleFloat, hb );
	}

	return !aimPoints.empty( );
}

void CRageBot::Multipoint( Vector& center, matrix3x4_t& matrix, std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& scale, int index ) {
	if ( hitbox->flRadius <= 0.f ) {
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMin.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup );

		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMax.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup );

		return;
	}

	if ( index ) {
		if ( index == 3 ) {
			if ( !RagebotStaticPointscale && scale > 0.9f )
				scale = 0.9f;
		}
		else {
			if ( index != 2
				&& index != 6 ) {
				if ( index == 4
					|| index == 5 ) {
					if ( !RagebotStaticPointscale && scale > 0.9f )
						scale = 0.9f;

					aimPoints.emplace_back( Math::VectorTransform( Vector( center.x, hitbox->vecBBMax.y - hitbox->flRadius * scale, center.z ), matrix ), hitbox->iGroup );
				}

				return;
			}

			if ( !RagebotStaticPointscale && scale > 0.9f )
				scale = 0.9f;

			if ( index == 6 ) {
				aimPoints.emplace_back( Math::VectorTransform( Vector( center.x, hitbox->vecBBMax.y - hitbox->flRadius * scale, center.z ), matrix ), hitbox->iGroup );

				return;
			}
		}

		return CalcCapsulePoints( aimPoints, hitbox, matrix, scale );
	}

	aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x + 0.70710678f * ( hitbox->flRadius * scale ), hitbox->vecBBMax.y - 0.70710678f * ( hitbox->flRadius * scale ), hitbox->vecBBMax.z ), matrix ), hitbox->iGroup );

	aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z + hitbox->flRadius * scale ), matrix ), hitbox->iGroup );

	aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z - hitbox->flRadius * scale ), matrix ), hitbox->iGroup );

	aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y - hitbox->flRadius * scale, hitbox->vecBBMax.z ), matrix ), hitbox->iGroup );
}

void CRageBot::CalcCapsulePoints( std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* const hitbox, matrix3x4_t& matrix, float scale ) {
	const auto min = Math::VectorTransform( hitbox->vecBBMin, matrix );
	const auto max = Math::VectorTransform( hitbox->vecBBMax, matrix );

	static matrix3x4_t matrix0 = Math::VectorMatrix( { 0.f, 0.f, 1.f } );
	const matrix3x4_t matrix1 = Math::VectorMatrix( ( max - min ).Normalized( ) );

	for ( const auto& vertices : {
		Vector{ 0.95f, 0.f, 0.f },
		Vector{ -0.95f, 0.f, 0.f },
		Vector{ 0.f, 0.95f, 0.f },
		Vector{ 0.f, -0.95f, 0.f },
		Vector{ 0.f, 0.f, 0.95f },
		Vector{ 0.f, 0.f, -0.95f }
		} ) {
		Vector point{ };

		Math::VectorIRotate( vertices, matrix0, point );
		Math::VectorIRotate( point, matrix1, point );

		point *= scale;

		if ( vertices.z > 0.f )
			point += min - max;

		aimPoints.emplace_back( point + max, hitbox->iGroup );
	}
}

void CRageBot::ScanPoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, AimPoint_t& point ) {
	if ( point.m_bScanned )
		return;

	point.m_bScanned = true;

	const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
		ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
		ctx.m_vecEyePos, point.m_vecPoint, RagebotAutowall ) };
		

	point.m_flDamage = data.dmg;
	point.m_iHitgroup = data.hitgroup;

	if ( std::find( m_iHitboxes.begin( ), m_iHitboxes.end( ), data.hitbox ) == m_iHitboxes.end( ) )
		return;

	point.m_bPenetrated = data.penetrationCount < 4;

	int mindmg{ point.m_bPenetrated ? RagebotPenetrationDamage : RagebotMinimumDamage };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = RagebotOverrideDamage;
	else if ( RagebotScaleDamage )
		mindmg *= player->m_iHealth( ) / 100.f;

	if ( point.m_flDamage < mindmg )
		return;

	point.m_iIntersections = record->m_bMultiMatrix ? SafePoint( player, record, point.m_vecPoint, data.hitbox ) : 3;

	point.m_bValid = true;
}

std::optional< AimPoint_t> CRageBot::PickPoints( CBasePlayer* player, std::vector<AimPoint_t>& aimPoints ) {
	std::optional< AimPoint_t> point{ };

	for ( const auto& p : aimPoints ) {
		if ( !p.m_bValid )
			continue;


		if ( !point.has_value( ) ) {
			point = p;
			continue;
		}

		// headpoints are scanned last
		if ( p.m_iHitgroup == HITGROUP_HEAD ) {
			// prefer baim
			if ( RagebotPreferBaim && point->m_iHitgroup != HITGROUP_HEAD )
				break;

			if ( RagebotPreferBaimLethal && point->m_flDamage > player->m_iHealth( ) && point->m_iHitgroup != HITGROUP_HEAD )
				break;

			// prefer baim if dt
			if ( RagebotPreferBaimDoubletap && ( Features::Exploits.m_iShiftAmount || ctx.m_iTicksAllowed ) )
				break;
		}

		// always prefer intersections
		if ( p.m_iIntersections >= point->m_iIntersections ) {
			if ( p.m_flDamage >= point->m_flDamage || ( p.m_flDamage > player->m_iHealth( ) && p.m_iIntersections > point->m_iIntersections ) )
				point = p;
		}
	}

	return point;
}

void CRageBot::Fire( CUserCmd& cmd ) {
	auto target = PickTarget( );
	if ( !target.has_value( ) )
		return;

	m_bShouldStop = true;

	if ( !ctx.m_bCanShoot )
		return;

	const auto backup = std::make_unique< LagBackup_t >( target->m_pPlayer );

	target->m_pRecord->Apply( target->m_pPlayer );
	QAngle angle;
	Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	{
		const auto backupPoseParam{ ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) };

		ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = ( std::clamp( std::remainder(
			angle.x
			- ctx.m_pLocal->m_aimPunchAngle( ).x * Offsets::Cvars.weapon_recoil_scale->GetFloat( ), 360.f
		), -90.f, 90.f ) + 90.f ) / 180.f;

		ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( );
		Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

		ctx.m_pLocal->m_flPoseParameter( ).at( 12u ) = backupPoseParam;
	}

	const int hitchance = ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : RagebotHitchance;

	if ( HitChance( target->m_pPlayer, angle, hitchance, target->m_cAimPoint->m_iHitgroup ) ) {
		if ( RagebotAutoFire )
			cmd.iButtons |= IN_ATTACK;

		if ( cmd.iButtons & IN_ATTACK ) {
			cmd.viewAngles = angle;

			m_bShouldStop = false;
			//cmd.flSideMove = Features::Misc.m_ve2OldMovement.y;
			//cmd.flForwardMove = Features::Misc.m_ve2OldMovement.x;

			if ( !RagebotSilentAim )
				Interfaces::Engine->SetViewAngles( angle );

			cmd.iTickCount = TIME_TO_TICKS( target->m_pRecord->m_cAnimData.m_flSimulationTime + ctx.m_flLerpTime );

			auto& entry{ Features::AnimSys.m_arrEntries.at( ( target->m_pPlayer->Index( ) - 1 ) ) };
			if ( !entry.m_iMissedShots )
				entry.m_iFirstResolverSide = target->m_pRecord->m_iResolverSide;

			/*ctx.doubletapping = false;*/

			// dont shoot at the same record we just shot at
			//target->m_pRecord->m_bAnimated = false;

			Features::Shots.add( ctx.m_vecEyePos, target->m_pRecord, cmd.iCommandNumber, target->m_pPlayer, target->m_cAimPoint->m_iHitgroup, target->m_cAimPoint->m_vecPoint );


			if ( Config::Get<bool>( Vars.MiscHitMatrix ) )
				Features::Visuals.Chams.AddHitmatrix( target->m_pPlayer, target->m_pRecord->m_cAnimData.m_cAnimSides.at( target->m_pRecord->m_iResolverSide ).m_pMatrix );

			const std::string message =
				_( "shot " ) + ( std::string )Interfaces::Engine->GetPlayerInfo( target->m_pPlayer->Index( ) )->szName +
				_( " | hitgroup: " ) + Hitgroup2Str( target->m_cAimPoint->m_iHitgroup ) +
				_( " | pred damage: " ) + std::to_string( int( target->m_cAimPoint->m_flDamage ) ).c_str( ) +
				_( " | backtrack: " ) + std::to_string( Interfaces::ClientState->iServerTick - target->m_pRecord->m_iReceiveTick ) + _( " ticks" ) +
				_( " | resolver side: " ) + std::to_string( target->m_pRecord->m_iResolverSide ) +
				_( " | simulated: " ) + std::to_string( target->m_pRecord->m_iNewCmds ) +
				_( " | safety: " ) + std::to_string( ( target->m_cAimPoint->m_iIntersections ) );

			Features::Logger.Log( message, false );
		}
	}
	

	backup->Apply( target->m_pPlayer );
}

bool CRageBot::HitChance( CBasePlayer* player, const QAngle& ang, int hitchance, int group ) {
	const int accuracy = static_cast< int >( std::ceilf( 128.f * ( hitchance * 0.01f ) ) );
	int total_hits = 0;

	Vector fwd{ }, right{ }, up{ };
	Math::AngleVectors( ang, &fwd, &right, &up );

	const auto item_index = ctx.m_pWeapon->m_iItemDefinitionIndex( );
	const auto recoil_index = ctx.m_pWeapon->m_flRecoilIndex( );

	for ( int i = 0u; i < 128u; ++i ) {
		const auto spread_angle = CalcSpreadAngle( item_index, recoil_index, i );

		const auto dir = ( fwd + ( right * spread_angle.x ) + ( up * spread_angle.y ) ).Normalized( );

		const Vector end = ctx.m_vecEyePos + dir * ctx.m_pWeaponData->flRange;

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, end, RagebotAutowall ) };

		if ( data.dmg > 0 ) {
			if ( !RagebotHitchanceThorough || data.hitgroup == group )
				total_hits++;
		}

		if ( total_hits >= accuracy )
			return true;

		// no chance, sorry bud
		if ( 128u - i < accuracy - total_hits )
			return false;
	}

	return false;
}

Vector2D CRageBot::CalcSpreadAngle( const int item_index, const float recoil_index, const int i ) {
	Math::RandomSeed( i + 1u );

	auto v1 = Math::RandomFloat( 0.0f, 1.0f );
	auto v2 = Math::RandomFloat( 0.0f, M_2PI );

	auto v3 = Math::RandomFloat( 0.0f, 1.0f );
	auto v4 = Math::RandomFloat( 0.0f, M_2PI );

	if ( recoil_index < 3.f && item_index == WEAPON_NEGEV ) {
		for ( auto i = 3; i > recoil_index; --i ) {
			v1 *= v1;
			v3 *= v3;
		}

		v1 = 1.f - v1;
		v3 = 1.f - v3;
	}

	if ( item_index == WEAPON_REVOLVER ) {
		v1 = 1.f - v1 * v1;
		v3 = 1.f - v3 * v3;
	}

	const auto inac = v1 * Features::EnginePrediction.Inaccuracy;
	const auto spre = v3 * Features::EnginePrediction.Spread;

	return {
		std::cos( v2 ) * inac + std::cos( v4 ) * spre,
		std::sin( v2 ) * inac + std::sin( v4 ) * spre
	};
}

std::optional<AimTarget_t> CRageBot::PickTarget( ) {
	if ( m_cAimTargets.empty( ) )
		return std::nullopt;

	std::optional<AimTarget_t> retTarget{ };

	for ( auto& target : m_cAimTargets ) {
		if ( !target.m_cAimPoint.has_value( ) )
			continue;

		if ( !target.m_cAimPoint->m_bValid )
			continue;

		if ( !retTarget.has_value( ) ) {
			retTarget = target;
			continue;
		}

		switch ( Config::Get<int>( Vars.RagebotTargetSelection ) )
		{
		case 0: {// highest damage
			if ( target.m_cAimPoint->m_flDamage > retTarget->m_cAimPoint->m_flDamage )
				retTarget = target;
		}break;
		case 1: // fov
		{
			const auto first_fov = Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, target.m_pPlayer->GetAbsOrigin( ) ) );
			const auto second_fov = Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, retTarget->m_pPlayer->GetAbsOrigin( ) ) );
			if ( first_fov < second_fov )
				retTarget = target;

			break;
		}
		case 2: // lowest distance
		{
			const float delta1 = target.m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) );
			const float delta2 = retTarget->m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) );

			if ( delta1 < delta2 )
				retTarget = target;

			break;
		}
		case 3: // lowest health
		{
			if ( target.m_pPlayer->m_iHealth( ) < retTarget->m_pPlayer->m_iHealth( ) )
				retTarget = target;

			break;
		}
		}
	}

	return retTarget;
}

// check if this is actually working
int CRageBot::SafePoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, Vector aimpoint, int index ) {
	const auto hitboxSet = player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return 0;

	const auto hitbox = hitboxSet->GetHitbox( index );
	if ( !hitbox )
		return 0;

	QAngle angle;
	Math::VectorAngles( aimpoint - ctx.m_vecEyePos, angle );

	Vector forward;
	Math::AngleVectors( angle, &forward );

	const Vector end = ctx.m_vecEyePos + forward * ctx.m_pWeaponData->flRange;

	int hits = 0;
	for ( int i = 0; i <= 2; i++ ) {
		Vector mins{ };
		Vector maxs{ };
		const auto& matrix{ record->m_cAnimData.m_cAnimSides.at( i ).m_pMatrix[ hitbox->iBone ] };

		mins = Math::VectorTransform( hitbox->vecBBMin, matrix );
		maxs = Math::VectorTransform( hitbox->vecBBMax, matrix );

		if ( hitbox->flRadius < 0 ) {
			Math::VectorITransform( ctx.m_vecEyePos, matrix, mins );
			Math::VectorITransform( aimpoint, matrix, maxs );
		}
		else {
			if ( Math::SegmentToSegment( ctx.m_vecEyePos, end, mins, maxs ) < hitbox->flRadius )
				++hits;

			continue;
		}


		if ( Math::IntersectionBoundingBox( ctx.m_vecEyePos, end, mins, maxs ) )
			++hits;
	}

	return hits;
}

int CRageBot::OffsetDelta( CBasePlayer* player, std::shared_ptr<LagRecord_t> record ) {
	QAngle shootAngle;

	const auto eye_pos = record->m_cAnimData.m_vecOrigin + player->m_vecViewOffset( );

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
	const auto hitbox = hitboxSet->GetHitbox( HITBOX_PELVIS );
	if ( !hitbox )
		return 90;

	const auto point = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ HITBOX_PELVIS ] );

	Math::VectorAngles( point - eye_pos, shootAngle );

	auto delta = Math::AngleDiff( record->m_angEyeAngles.y, shootAngle.y );

	for ( ; delta > 90; delta -= 180 );
	for ( ; delta < -90; delta += 180 );

	return abs( delta );
}

bool CRageBot::CheckHeadSafepoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record ) {
	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
	const auto hitbox = hitboxSet->GetHitbox( HITBOX_HEAD );
	if ( !hitbox )
		return false;

	const auto point = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ HITBOX_HEAD ] );

	return SafePoint( player, record, point, HITBOX_HEAD ) >= 3;
}

std::shared_ptr<LagRecord_t> CRageBot::GetBestLagRecord( PlayerEntry& entry ) {
	if ( entry.m_pRecords.empty( ) )
		return nullptr;

	std::shared_ptr<LagRecord_t> bestRecord{ };

	int bestDamage{ };
	int bestEyedelta{ INT_MAX };
	bool dontScan{ };
	const auto backup = std::make_unique< LagBackup_t >( entry.m_pPlayer );

	const int pingTicks{ TIME_TO_TICKS( ctx.m_flOutLatency ) };

	auto predServerAngle{ entry.m_pPlayer->m_angEyeAngles( ).y };
	// wtf is the maths equation for this?
	/*if ( pingTicks > 0
		&& pingTicks >= entry.m_iRealChoked ) {
		bool tog{ };
		for ( int i{ }; i <= pingTicks; ++i ) {
			if ( i % entry.m_iRealChoked == 0 )
				tog = !tog;
		}

		if ( !tog )
			predServerAngle += entry.m_flJitterAmount;
	}

	if ( std::find( entry.m_pRecords.begin( ), entry.m_pRecords.end( ), 
		[ & ]( const std::shared_ptr< LagRecord_t >& lag_record ) -> bool {
			return lag_record->m_bBrokeLC; } 
	)  == entry.m_pRecords.end( ) )*/

	if ( const auto& record{ entry.m_pRecords.back( ) }; record->m_bBrokeLC ) {
		if ( entry.m_vecUpdatedOrigin != entry.m_pPlayer->m_vecOrigin( ) )
			return nullptr;

		if ( !record->m_bAnimated )//!record->IsValid( ) || don tneed
			return nullptr;

		record->SelectResolverSide( entry );

		const int dmg{ QuickScan( entry.m_pPlayer, record, dontScan ) };

		if ( dmg > 0 )
			return record;
	}

	for ( auto it = entry.m_pRecords.rbegin( ); it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };
		if ( !record )
			continue;

		if ( record->m_bBrokeLC )
			break;

		if ( !record->IsValid( )
			|| !record->m_bAnimated )
			continue;

		record->SelectResolverSide( entry );

		const int dmg{ QuickScan( entry.m_pPlayer, record, dontScan ) };
		const auto eyeDelta{ std::abs( Math::AngleDiff( record->m_angEyeAngles.y, predServerAngle ) ) };

		if ( dmg > bestDamage ) {
			bestDamage = dmg;
			bestRecord = record;
			bestEyedelta = eyeDelta;
		}

		if ( dontScan ) {
			bool metScaled{ };
			if ( eyeDelta < 15.f ) {
				if ( !dmg ) {
					QuickScan( entry.m_pPlayer, record, metScaled );
					if ( metScaled ) {
						bestRecord = record;
						break;
					}
				}
				else {
					bestRecord = record;
					break;
				}
			}
			else if ( eyeDelta < bestEyedelta ) {
				if ( !dmg ) {
					QuickScan( entry.m_pPlayer, record, metScaled );
					if ( metScaled ) {
						bestRecord = record;
						bestEyedelta = eyeDelta;
					}
				}
			}
		}
	}

	backup->Apply( entry.m_pPlayer );

	return bestRecord;
}

int CRageBot::QuickScan( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, bool& metScaled ) {
	if ( metScaled )
		return 0;

	record->Apply( player );

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
	const auto& side{ record->m_cAnimData.m_cAnimSides.at( record->m_iResolverSide ) };

	int dmg{ };

	for ( const auto& hb : { HITBOX_STOMACH, HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {// opt
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, side.m_pMatrix[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, point, RagebotAutowall ) };

		if ( hb == HITBOX_STOMACH || hb == HITBOX_HEAD ) {
			if ( Features::Autowall.ScaleDamage( player, ctx.m_pWeaponData->iDamage,
				ctx.m_pWeaponData->flArmorRatio, data.hitgroup ) - data.dmg <= 5.f
				|| ( data.dmg >= 110 ) )
				metScaled = true;
		}

		dmg += data.dmg;
	}
	return dmg;
}