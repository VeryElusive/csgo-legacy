#include "ragebot.h"
#include "exploits.h"
#include "../visuals/visuals.h"
#include "../../utils/threading/threading.h"

// TODO: are these vector rotate shit right?
void CRageBot::Main( CUserCmd& cmd ) {
	Reset( );

	if ( !Config::Get<bool>( Vars.RagebotEnable )
		|| !ctx.m_pWeapon || !ctx.m_pWeaponData
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
		if ( !player || player == ctx.m_pLocal || !player->IsPlayer( ) || player->m_bGunGameImmunity( ) || player->IsTeammate( ) || !player->m_pAnimState( ) )
			continue;

		const auto fov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) ) };
		if ( fov > RagebotFOV )
			continue;

		auto& entry{ Features::AnimSys.m_arrEntries.at( i - 1 ) };
		if ( entry.m_pPlayer != player )
			continue;

		const auto record{ Features::Ragebot.GetBestLagRecord( entry ) };
		if ( !record )
			continue;

		m_cAimTargets.push_back( { record, entry.m_pPlayer, entry.m_iMissedShots } );
	}

	ScanTargets( );
	Fire( cmd );

	m_iHitboxes.clear( );
	m_cAimTargets.clear( );
}

void CRageBot::ScanTargets( ) {
	for ( auto& target : m_cAimTargets ) {
		if ( !CreatePoints( target, target.m_cPoints ) )
			continue;

		const auto backup{ std::make_unique< LagBackup_t >( target.m_pPlayer ) };

		target.m_pRecord->Apply( target.m_pPlayer );

		for ( auto& point : target.m_cPoints )
			ScanPoint( target.m_pPlayer, target.m_pRecord, point );

		backup->Apply( target.m_pPlayer );
	}

	m_cAimTargets.erase(
		std::remove_if(
			m_cAimTargets.begin( ), m_cAimTargets.end( ),
			[ & ]( AimTarget_t& target ) {
				target.m_cAimPoint = PickPoints( target.m_pPlayer, target.m_cPoints );
				return !target.m_cAimPoint.has_value( );
			}
		),
		m_cAimTargets.end( )
				);
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
			if ( hb == HITBOX_HEAD )
				ret += 4;
			else
				ret += 2;
		}

	}

	return ret;
}

bool CRageBot::CreatePoints( AimTarget_t& aimTarget, std::vector<AimPoint_t>& aimPoints ) {
	const auto hitboxSet = aimTarget.m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( aimTarget.m_pPlayer->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	aimPoints.reserve( CalcPointCount( hitboxSet ) );

	for ( const auto& hb : m_iHitboxes ) {
		const auto hitbox{ hitboxSet->GetHitbox( hb ) };
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

		auto& matrix{ aimTarget.m_pRecord->m_pMatrix[ hitbox->iBone ] };

		const auto center{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, matrix ) };

		aimPoints.emplace_back( center, hitbox->iGroup, center );

		// opt
		if ( IsMultiPointEnabled( hb ) /* && point.m_flDamage > 0*/ )
			Multipoint( center, matrix, aimPoints, hitbox, hitboxSet, scaleFloat, hb );
	}

	return !aimPoints.empty( );
}

// TODO: multipoints
void CRageBot::Multipoint( const Vector& center, matrix3x4_t& matrix, std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& scale, int index ) {
	if ( hitbox->flRadius <= 0.f ) {
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMin.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup, center );

		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMax.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup, center );

		return;
	}


	const auto min{ Math::VectorTransform( hitbox->vecBBMin, matrix ) };
	const auto max{ Math::VectorTransform( hitbox->vecBBMax, matrix ) };

	const auto maxMin{ ( max - min ).Normalized( ) };

	auto delta{ ( center - ctx.m_vecEyePos ) };
	auto cr{ maxMin.CrossProduct( delta ) };

	Vector right{ }, up{ };
	if ( index == HITBOX_HEAD ) {
		QAngle cr_angle{ }, tmp{ };

		Math::VectorAngles( cr, cr_angle );
		Math::AngleVectors( cr_angle, nullptr, &right, &up );

		Math::VectorAngles( delta, tmp );
		cr_angle.x = tmp.x;

		right = cr;
	}
	else
		Math::AngleVectors( { delta.x, delta.y, delta.z }, nullptr, &right, &up );

	RayTracer::Hitbox box( min, max, hitbox->flRadius );
	RayTracer::Trace trace;

	if ( index == HITBOX_HEAD ) {
		Vector middle = ( right.Normalized( ) + up.Normalized( ) ) * 0.5f;
		Vector middle2 = ( right.Normalized( ) - up.Normalized( ) ) * 0.5f;

		RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( middle * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );

		ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( middle2 * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );

		ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );

		ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );
	}
	else {
		RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );

		ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale,
			hitbox->iGroup, center );
	}
}

void CRageBot::ScanPoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, AimPoint_t& point ) {
	if ( point.m_bScanned )
		return;

	point.m_bScanned = true;

	const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
		ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
		ctx.m_vecEyePos, point.m_vecPoint, RagebotAutowall ) };

	if ( static_cast<int>( data.dmg ) < 1 )
		return;

	point.m_flDamage = static_cast< int >( data.dmg );
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

	//point.m_iIntersections = record->m_bMultiMatrix ? SafePoint( player, record, point.m_vecPoint, data.hitbox ) : 3;
	m_bShouldStop = true;
	point.m_bValid = true;
}

std::optional< AimPoint_t> CRageBot::PickPoints( CBasePlayer* player, std::vector<AimPoint_t>& aimPoints ) {
	std::optional< AimPoint_t> point{ };

	const int hitchance = ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : RagebotHitchance;

	for ( const auto& p : aimPoints ) {
		if ( !p.m_bValid )
			continue;

		if ( !point.has_value( ) ) {
			point = p;
			continue;
		}

		// headpoints are scanned last
		if ( p.m_iHitgroup == HITGROUP_HEAD ) {
			//if ( !FastHitChance( player, p.m_vecPoint, mins, maxs, headHB->flRadius, hitchance ) )
			//	continue;

			// prefer baim
			if ( RagebotPreferBaim && point->m_iHitgroup != HITGROUP_HEAD )
				break;

			if ( RagebotPreferBaimLethal && point->m_flDamage > player->m_iHealth( ) && point->m_iHitgroup != HITGROUP_HEAD )
				break;

			// prefer baim if dt
			if ( RagebotPreferBaimDoubletap && ( Features::Exploits.m_iShiftAmount || ctx.m_iTicksAllowed ) )
				break;
		}

		// safer point for middle
		if ( p.m_flCenterAmt <= point->m_flCenterAmt ) {
			if ( p.m_flDamage >= point->m_flDamage || ( p.m_flDamage > player->m_iHealth( ) && p.m_flCenterAmt < point->m_flCenterAmt ) )
				point = p;
		}
	}

	return point;
}

void CRageBot::Fire( CUserCmd& cmd ) {
	auto target = PickTarget( );
	if ( !target.has_value( ) )
		return;

	if ( !ctx.m_bCanShoot )
		return;

	const auto backup = std::make_unique< LagBackup_t >( target->m_pPlayer );

	target->m_pRecord->Apply( target->m_pPlayer );
	QAngle angle;
	Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( angle.x );

	Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	const int hitchance{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : RagebotHitchance };

	const auto& max{ target->m_pPlayer->m_vecMaxs( ) };

	//target->m_pPlayer->SetCollisionBounds( target->m_pPlayer->m_vecMins( ), { max.x, max.y, max.z + 5 } );
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

			/*auto& entry{ Features::AnimSys.m_arrEntries.at( ( target->m_pPlayer->Index( ) - 1 ) ) };
			if ( !entry.m_iMissedShots )
				entry.m_iFirstResolverSide = target->m_pRecord->m_iResolverSide;*/

			// dont shoot at the same record we just shot at
			//target->m_pRecord->m_bAnimated = false;

			Features::Shots.add( ctx.m_vecEyePos, target->m_pRecord, cmd.iCommandNumber, target->m_pPlayer, target->m_cAimPoint->m_iHitgroup, target->m_cAimPoint->m_vecPoint );


			if ( Config::Get<bool>( Vars.MiscHitMatrix ) )
				Features::Visuals.Chams.AddHitmatrix( target->m_pPlayer, target->m_pRecord->m_pMatrix );

			const std::string message =
				_( "shot " ) + ( std::string )Interfaces::Engine->GetPlayerInfo( target->m_pPlayer->Index( ) )->szName +
				_( " | hitgroup: " ) + Hitgroup2Str( target->m_cAimPoint->m_iHitgroup ) +
				_( " | pred damage: " ) + std::to_string( int( target->m_cAimPoint->m_flDamage ) ).c_str( ) +
				_( " | backtrack: " ) + std::to_string( Interfaces::ClientState->iServerTick - target->m_pRecord->m_iReceiveTick ) + _( " ticks" ) +
				_( " | simulated: " ) + std::to_string( target->m_pRecord->m_iNewCmds );// +
				//_( " | safety: " ) + std::to_string( ( target->m_cAimPoint->m_iIntersections ) );

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

	for ( int i{ }; i < 128u; ++i ) {
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

int CRageBot::OffsetDelta( CBasePlayer* player, std::shared_ptr<LagRecord_t> record ) {
	QAngle shootAngle;

	auto eye_pos = record->m_cAnimData.m_vecOrigin;
	eye_pos.z += 64.f;

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
	const auto hitbox = hitboxSet->GetHitbox( HITBOX_CHEST );
	if ( !hitbox )
		return 90;

	const auto point = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ HITBOX_CHEST ] );

	Math::VectorAngles( point - eye_pos, shootAngle );

	auto delta = Math::AngleDiff( record->m_angEyeAngles.y, shootAngle.y );

	for ( ; delta > 90; delta -= 180 );
	for ( ; delta < -90; delta += 180 );

	return abs( delta );
}

std::shared_ptr<LagRecord_t> CRageBot::GetBestLagRecord( PlayerEntry& entry ) {
	if ( entry.m_pRecords.empty( ) )
		return nullptr;

	std::shared_ptr<LagRecord_t> bestRecord{ };

	const auto backup = std::make_unique< LagBackup_t >( entry.m_pPlayer );

	const int pingTicks{ TIME_TO_TICKS( ctx.m_flOutLatency ) };

	if ( const auto& record{ entry.m_pRecords.back( ) }; record->m_bBrokeLC ) {
		if ( record->m_cAnimData.m_vecOrigin != entry.m_pPlayer->m_vecOrigin( ) )
			return nullptr;

		if ( !record->m_bAnimated
			|| pingTicks >= entry.m_iRealChoked )//!record->IsValid( ) || don tneed
			return nullptr;

		bool a{ };
		const int dmg{ QuickScan( entry.m_pPlayer, record, a ) };

		if ( dmg > 0 )
			return record;
		
		return nullptr;
	}

	int bestDamage{ };
	int bestEyedelta{ INT_MAX };

	for ( auto it = entry.m_pRecords.rbegin( ); it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };

		if ( record->m_bBrokeLC )
			break;

		if ( !record->IsValid( )
			|| !record->m_bAnimated )
			continue;

		bool metScaled{ };

		const int dmg{ QuickScan( entry.m_pPlayer, record, metScaled ) };

		if ( dmg > bestDamage ) {
			bestDamage = dmg;
			bestRecord = record;
		}

		if ( metScaled && record->m_bBrokeLBY ) {
			bestRecord = record;
			break;
		}
	}

	backup->Apply( entry.m_pPlayer );

	return bestRecord;
}

int CRageBot::QuickScan( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, bool& metScaled ) {
	record->Apply( player );

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };

	int dmg{ };

	for ( const auto& hb : { HITBOX_STOMACH, HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {// opt
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, record->m_pMatrix[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, point, RagebotAutowall ) };

		if ( static_cast< int >( data.dmg ) < 1 )
			continue;

		if ( hb == HITBOX_STOMACH || hb == HITBOX_HEAD ) {
			float dmg{ static_cast< float >( ctx.m_pWeaponData->iDamage ) };
			Features::Autowall.ScaleDamage( player, dmg,
				ctx.m_pWeaponData->flArmorRatio, data.hitgroup, 4.f );//ctx.m_pWeaponData->flHeadShotMultiplier

			if ( dmg - data.dmg <= 5.f
				|| ( data.dmg >= 110 ) )
				metScaled = true;
		}

		dmg += data.dmg;
	}
	return dmg;
}