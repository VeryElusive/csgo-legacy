#include "ragebot.h"
#include "exploits.h"
#include "../visuals/visuals.h"

void CRageBot::Main( CUserCmd& cmd ) {
	Reset( );
	//if ( ctx.m_strDbgLogs.size( ) )
	//	ctx.m_strDbgLogs.clear( );

	if ( !ctx.m_pWeapon || !ctx.m_pWeaponData )
		return;

	if ( ctx.m_pWeaponData->nWeaponType != m_iLastWeaponType
		|| ctx.m_pWeapon->m_iItemDefinitionIndex( ) != m_iLastWeaponIndex
		|| Menu::Opened )
		ParseCfgItems( ctx.m_pWeaponData->nWeaponType );

	if ( !Config::Get<bool>( Vars.RagebotEnable )
		//|| ctx.m_pWeapon->m_bReloading( ) TODO: LEGACY!
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

		//const auto dbg{ &ctx.m_strDbgLogs.emplace_back( ) }; 

		m_cAimTargets.emplace_back( record, entry );
	}

	ScanTargets( );
	Fire( cmd );

	//for ( auto& target : m_cAimTargets )
	//	target.m_pDbgLog->append( " << " + ( std::string )Interfaces::Engine->GetPlayerInfo( target.m_pPlayer->Index( ) )->szName );

	m_iHitboxes.clear( );
	m_cAimTargets.clear( );
}

void CRageBot::ScanTargets( ) {
	for ( auto& target : m_cAimTargets ) {
		const auto backup{ std::make_unique< LagBackup_t >( target.m_pPlayer ) };

		target.m_pRecord->Apply( target.m_pPlayer, false );
		//*target.m_pDbgLog = _( "CP" );

		if ( !CreatePoints( target ) ) {
			backup->Apply( target.m_pPlayer );
			continue;
		}

		for ( auto& point : target.m_cPoints )
			ScanPoint( target, point );

		//*target.m_pDbgLog = _( "SP" );

		backup->Apply( target.m_pPlayer );

		target.m_cAimPoint = PickPoints( target.m_pPlayer, target.m_cPoints );
	}


	/*m_cAimTargets.erase(
		std::remove_if(
			m_cAimTargets.begin( ), m_cAimTargets.end( ),
			[ & ]( AimTarget_t& target ) {
				target.m_cAimPoint = PickPoints( target.m_pPlayer, target.m_cPoints );
				return !target.m_cAimPoint;
			}
		),
		m_cAimTargets.end( )
				);*/
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

		if ( hitbox->flRadius <= 0.f ) {
			ret += 2;
			continue;
		}

		switch ( hb ) {
		case HITBOX_HEAD:
			ret += 4;
			break;
		case HITBOX_PELVIS:
		case HITBOX_STOMACH:
			ret += 3;
			break;
		case HITBOX_CHEST:
		case HITBOX_LOWER_CHEST:
		case HITBOX_UPPER_CHEST:
			ret++;
			break;
		case HITBOX_RIGHT_THIGH:
		case HITBOX_LEFT_THIGH:
			ret++;
			break;
		}
	}

	return ret;
}

FORCEINLINE int CRageBot::OffsetDelta( CBasePlayer* player, LagRecord_t* record ) {
	QAngle shootAngle;

	auto eyePos = record->m_cAnimData.m_vecOrigin;
	eyePos.z += 64.f;

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
	const auto hitbox = hitboxSet->GetHitbox( HITBOX_CHEST );
	if ( !hitbox )
		return 90;

	const auto point = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ HITBOX_CHEST ] );

	Math::VectorAngles( point - eyePos, shootAngle );

	auto delta = Math::AngleDiff( record->m_angEyeAngles.y, shootAngle.y );

	for ( ; delta > 90; delta -= 180 );
	for ( ; delta < -90; delta += 180 );

	return std::abs( delta );
}

bool CRageBot::CreatePoints( AimTarget_t& aimTarget ) {
	const auto hitboxSet = aimTarget.m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( aimTarget.m_pPlayer->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	const auto& missed = aimTarget.m_iMissedShots;

	aimTarget.m_cPoints.reserve( CalcPointCount( hitboxSet ) );

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
			scale -= std::floor( std::min( Features::EnginePrediction.Inaccuracy + Features::EnginePrediction.Spread, 0.15f ) * 333.333f );

			// now compare side delta
			scale -= std::min( 90 - std::abs( 90 - OffsetDelta( aimTarget.m_pPlayer, aimTarget.m_pRecord.get( ) ) ), scale );
		}

		float scaleFloat{ scale / 100.f };

		if ( !( aimTarget.m_pPlayer->m_iEFlags( ) && FL_ONGROUND ) )
			scale = std::min( scaleFloat, 0.7f );

		auto& matrix{ aimTarget.m_pRecord->m_cAnimData.m_pMatrix[ hitbox->iBone ] };

		Vector center{ ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f };
		center = Math::VectorTransform( center, matrix );

		aimTarget.m_cPoints.emplace_back( center, hitbox->iGroup, center );

		if ( IsMultiPointEnabled( hb ) /* && point.m_flDamage > 0*/ )
			Multipoint( center, matrix, aimTarget.m_cPoints, hitbox, hitboxSet, scaleFloat, hb );
	}

	return !aimTarget.m_cPoints.empty( );
}

void CRageBot::Multipoint( Vector& center, matrix3x4_t& matrix, std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float scale, int index ) {
	if ( hitbox->flRadius <= 0.f ) {
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMin.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup, center );

		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x + ( hitbox->vecBBMax.x - center.x ) * scale, center.y, center.z ), matrix ),
			hitbox->iGroup, center );

		return;
	}

#define r ( hitbox->flRadius * scale )

	switch ( index ) {
	case HITBOX_HEAD: {
		scale = std::clamp<float>( scale, 0.1f, 0.9f );

		// std::cos( DEG2RAD( 45.f ) ) = 0.70710678f
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x + ( 0.70710678f * r ), hitbox->vecBBMax.y + ( -0.70710678f * r ), hitbox->vecBBMax.z ), matrix ),
			hitbox->iGroup, center );

		// back
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x, hitbox->vecBBMax.y - r, center.z ), matrix ),
			hitbox->iGroup, center );

		// left
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z - r ), matrix ),
			hitbox->iGroup, center );

		// right
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z + r ), matrix ),
			hitbox->iGroup, center );
	}break;

	case HITBOX_PELVIS:
	case HITBOX_STOMACH: {
		// back
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x, hitbox->vecBBMax.y - r, center.z ), matrix ),
			hitbox->iGroup, center );

		// right
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z + r ), matrix ),
			hitbox->iGroup, center );

		// left
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x, hitbox->vecBBMax.y, hitbox->vecBBMax.z - r ), matrix ),
			hitbox->iGroup, center );
	}break;
	case HITBOX_CHEST:
	case HITBOX_LOWER_CHEST:
	case HITBOX_UPPER_CHEST: {
		if ( index != HITBOX_UPPER_CHEST ) {
			if ( scale > 0.9f )
				scale = 0.9f;
		}
		// back
		aimPoints.emplace_back( Math::VectorTransform( Vector( center.x, hitbox->vecBBMax.y - r, center.z ), matrix ),
			hitbox->iGroup, center );
	}break;
	case HITBOX_RIGHT_THIGH:
	case HITBOX_LEFT_THIGH: {
		// half bottom
		aimPoints.emplace_back( Math::VectorTransform( Vector( hitbox->vecBBMax.x - r, hitbox->vecBBMax.y, hitbox->vecBBMax.z ), matrix ),
			hitbox->iGroup, center );
	}break;
	}
#undef r
}

void CRageBot::ScanPoint( AimTarget_t& aim_target, AimPoint_t& point ) {
	if ( point.m_bScanned )
		return;

	point.m_bScanned = true;

	const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, aim_target.m_pPlayer, ctx.m_pWeaponData,
		ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
		ctx.m_vecEyePos, point.m_vecPoint, RagebotAutowall ) };

	if ( static_cast< int >( data.dmg ) < 1 )
		return;

	point.m_flDamage = static_cast< int >( data.dmg );
	point.m_iHitgroup = data.hitgroup;

	if ( std::find( m_iHitboxes.begin( ), m_iHitboxes.end( ), data.hitbox ) == m_iHitboxes.end( ) )
		return;

	//if ( data.hitbox != HITBOX_HEAD && point.m_iHitbox == HITBOX_HEAD )
	//	return;

	point.m_bPenetrated = data.penetrationCount < 4;

	int mindmg{ point.m_bPenetrated ? RagebotPenetrationDamage : RagebotMinimumDamage };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = RagebotOverrideDamage;
	else if ( RagebotScaleDamage )
		mindmg *= aim_target.m_pPlayer->m_iHealth( ) / 100.f;

	if ( point.m_flDamage < mindmg )
		return;

	m_bShouldStop = true;
	point.m_bValid = true;
}

AimPoint_t* CRageBot::PickPoints( CBasePlayer* player, std::vector<AimPoint_t>& aimPoints ) {
	AimPoint_t* bestPoint{ };

	//const int hitchance = ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : RagebotHitchance;

	for ( auto& point : aimPoints ) {
		if ( !point.m_bValid )
			continue;

		if ( !bestPoint ) {
			bestPoint = &point;
			continue;
		}

		// headpoints are scanned last
		if ( point.m_iHitgroup == HITGROUP_HEAD ) {
			//if ( !FastHitChance( player, p.m_vecPoint, mins, maxs, headHB->flRadius, hitchance ) )
			//	continue;

			// prefer baim
			if ( RagebotPreferBaim && bestPoint->m_iHitgroup != HITGROUP_HEAD )
				break;

			if ( RagebotPreferBaimLethal && bestPoint->m_flDamage > player->m_iHealth( ) && bestPoint->m_iHitgroup != HITGROUP_HEAD )
				break;

			// prefer baim if dt
			if ( RagebotPreferBaimDoubletap && ( Features::Exploits.m_iShiftAmount || ctx.m_iTicksAllowed ) )
				break;
		}

		// safer point for middle
		if ( point.m_flDamage == bestPoint->m_flDamage ) {
			if ( point.m_flCenterAmt < bestPoint->m_flCenterAmt )
				bestPoint = &point;
		}
		else if ( point.m_flDamage > bestPoint->m_flDamage )
			bestPoint = &point;
	}

	return bestPoint;
}

void CRageBot::Fire( CUserCmd& cmd ) {
	if ( !ctx.m_bCanShoot )
		return;

	const auto target{ PickTarget( ) };
	if ( !target )
		return;

	//*target->m_pDbgLog = _( "HC" );

	const auto backup{ std::make_unique< LagBackup_t >( target->m_pPlayer ) };

	target->m_pRecord->Apply( target->m_pPlayer, false );

	std::memcpy( target->m_pPlayer->m_CachedBoneData( ).Base( ), target->m_pRecord->m_cAnimData.m_pMatrix, target->m_pRecord->m_iBonesCount * sizeof( matrix3x4_t ) );


	QAngle angle;
	Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( angle.x );

	Math::VectorAngles( target->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	const int hitchance{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : RagebotHitchance };

	if ( HitChance( target->m_pPlayer, angle, hitchance, target->m_cAimPoint->m_iHitgroup ) ) {
		if ( RagebotAutoFire )
			cmd.iButtons |= IN_ATTACK;

		if ( cmd.iButtons & IN_ATTACK ) {
			cmd.viewAngles = angle - ctx.m_pLocal->m_aimPunchAngle( ) * Offsets::Cvars.weapon_recoil_scale->GetFloat( );

			if ( !RagebotSilentAim )
				Interfaces::Engine->SetViewAngles( angle );

			cmd.iTickCount = TIME_TO_TICKS( target->m_pRecord->m_cAnimData.m_flSimulationTime + ctx.m_flLerpTime );

			Features::Shots.add( ctx.m_vecEyePos, target->m_pRecord, cmd.iCommandNumber, 
				target->m_pPlayer, target->m_cAimPoint->m_iHitgroup, target->m_cAimPoint->m_vecPoint );

			if ( Config::Get<bool>( Vars.MiscHitMatrix ) ) {
				/*auto hdr = Interfaces::ModelInfo->GetStudioModel( target->m_pPlayer->GetModel( ) );

				if ( hdr ) {
					for ( int m{ }; m < 3; m++ ) {
						const auto matrix = target->m_pMatrices[ m ];
						auto hitboxSet = hdr->GetHitboxSet( target->m_pPlayer->m_nHitboxSet( ) );
						if ( hitboxSet ) {
							for ( int i{ }; i < hitboxSet->nHitboxes; ++i ) {
								const auto hitbox = hitboxSet->GetHitbox( i );
								if ( hitbox->flRadius <= 0.f )
									continue;

								const auto min = Math::VectorTransform( hitbox->vecBBMin, matrix[ hitbox->iBone ] );
								const auto max = Math::VectorTransform( hitbox->vecBBMax, matrix[ hitbox->iBone ] );

								Color color = m == 0 ? Color( 255, 0, 0 ) : m == 1 ? Color( 0, 255, 0 ) : Color( 0, 0, 255 );

								Interfaces::DebugOverlay->AddCapsuleOverlay( min, max, hitbox->flRadius, color[ 0 ], color[ 1 ], color[ 2 ], 50, 5, 0, 1 );
							}
						}
					}
				}*/

				Features::Visuals.Chams.AddHitmatrix( target->m_pPlayer, target->m_pRecord->m_cAnimData.m_pMatrix );
			}

			const std::string message =
				_( "shot " ) + ( std::string )Interfaces::Engine->GetPlayerInfo( target->m_pPlayer->Index( ) )->szName +
				_( " | hitgroup: " ) + Hitgroup2Str( target->m_cAimPoint->m_iHitgroup ) +
				_( " | pred damage: " ) + std::to_string( int( target->m_cAimPoint->m_flDamage ) ).c_str( ) +
				_( " | backtrack: " ) + std::to_string( Interfaces::ClientState->iServerTick - target->m_pRecord->m_iReceiveTick ) + _( " ticks" );

			Features::Logger.Log( message, false );
		}
	}
	backup->Apply( target->m_pPlayer );
}

bool CRageBot::HitChance( CBasePlayer* player, const QAngle& ang, int hitchance, int group ) {
	const int accuracy{ static_cast< int >( std::ceilf( 128.f * ( hitchance * 0.01f ) ) ) };
	int total_hits{ };

	Vector fwd{ }, right{ }, up{ };
	Math::AngleVectors( ang, &fwd, &right, &up );

	const auto item_index = ctx.m_pWeapon->m_iItemDefinitionIndex( );
	const auto recoil_index = ctx.m_pWeapon->m_flRecoilIndex( );

	for ( int i{ }; i < 128u; ++i ) {
		const auto spreadAngle{ CalcSpreadAngle( item_index, recoil_index, i ) };

		const auto dir{ ( fwd + ( right * spreadAngle.x ) + ( up * spreadAngle.y ) ).Normalized( ) };

		const auto end{ ctx.m_vecEyePos + dir * ctx.m_pWeaponData->flRange };

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

AimTarget_t* CRageBot::PickTarget( ) {
	if ( m_cAimTargets.empty( ) )
		return nullptr;

	AimTarget_t* retTarget{ };

	for ( auto& target : m_cAimTargets ) {
		if ( !target.m_cAimPoint )
			continue;

		//*target.m_pDbgLog = _( "PT" );

		if ( !retTarget ) {
			retTarget = &target;
			continue;
		}

		switch ( Config::Get<int>( Vars.RagebotTargetSelection ) )
		{
		case 0: {// highest damage
			if ( target.m_cAimPoint->m_flDamage > retTarget->m_cAimPoint->m_flDamage )
				retTarget = &target;
		}break;
		case 1: // fov
		{
			const auto firstFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, target.m_pPlayer->GetAbsOrigin( ) ) ) };
			const auto secondFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, retTarget->m_pPlayer->GetAbsOrigin( ) ) ) };
			if ( firstFov < secondFov )
				retTarget = &target;

			break;
		}
		case 2: // lowest distance
		{
			const float delta1{ target.m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };
			const float delta2{ retTarget->m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };

			if ( delta1 < delta2 )
				retTarget = &target;

			break;
		}
		case 3: // lowest health
		{
			if ( target.m_pPlayer->m_iHealth( ) < retTarget->m_pPlayer->m_iHealth( ) )
				retTarget = &target;

			break;
		}
		}
	}

	return retTarget;
}

FORCEINLINE bool sameRecord( const LagRecord_t* record, const LagRecord_t* prev ) {
	if ( !prev )
		return false;

	return record->m_angEyeAngles == prev->m_angEyeAngles
		&& record->m_cAnimData.m_vecVelocity == prev->m_cAnimData.m_vecVelocity
		&& record->m_cAnimData.m_flDuckAmount == prev->m_cAnimData.m_flDuckAmount
		&& record->m_cAnimData.m_vecMins == prev->m_cAnimData.m_vecMins
		&& record->m_cAnimData.m_vecMaxs == prev->m_cAnimData.m_vecMaxs
		&& record->m_cAnimData.m_vecOrigin == prev->m_cAnimData.m_vecOrigin;
}

std::shared_ptr< LagRecord_t > CRageBot::GetBestLagRecord( PlayerEntry& entry ) {
	if ( entry.m_pRecords.empty( ) )
		return nullptr;

	std::shared_ptr< LagRecord_t > bestRecord{ };
	std::shared_ptr< LagRecord_t > prevRecord{ };

	const auto backup = std::make_unique< LagBackup_t >( entry.m_pPlayer );

	if ( const auto& record{ entry.m_pRecords.back( ) }; record->m_bBrokeLC ) {
		if ( record->m_cAnimData.m_vecOrigin != entry.m_pPlayer->m_vecOrigin( ) )
			return nullptr;

		if ( !record->m_bAnimated )
			return nullptr;

		const auto timeDelta{ std::clamp(
			TIME_TO_TICKS(
				(
					( ctx.m_flRealOutLatency )
					+ Interfaces::Globals->flRealTime
					) - record->m_flReceiveTime
			),
			0, 100
		) };

		if ( timeDelta >= entry.m_pRecords.back( )->m_iNewCmds - 1 )
			return nullptr;

		bool a{ };
		const int dmg{ QuickScan( entry.m_pPlayer, record.get( ), a ) };

		if ( dmg > 0 )
			return record;

		return nullptr;
	}

	int bestDamage{ };

	for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };

		if ( record->m_bBrokeLC )
			break;

		if ( !record->IsValid( )
			|| !record->m_bAnimated )
			continue;

		if ( sameRecord( record.get( ), prevRecord.get( ) ) )
			continue;

		prevRecord = record;
		bool metScaled{ };

		const auto dmg{ QuickScan( entry.m_pPlayer, record.get( ), metScaled ) };

		if ( dmg > bestDamage + 10 
			|| !bestRecord
			|| bestRecord->m_cAnimData.m_vecVelocity.Length2D( ) <= 0.1f ) {
			if ( dmg > bestDamage ) {
				bestDamage = dmg;
				bestRecord = record;
			}
			else if ( dmg == bestDamage ) {
				bestDamage = dmg;
				bestRecord = record;
			}
		}

		if ( metScaled ) {
			if ( !bestRecord 
				|| ( bestRecord->m_cAnimData.m_vecVelocity.Length2D( ) <= 0.1f
				&& record->m_cAnimData.m_vecVelocity.Length2D( ) > 0.1f ) ) {
				bestRecord = record;
				break;
			}
			else if ( record->m_bBrokeLBY
				&& !bestRecord->m_bBrokeLBY ) {
				bestRecord = record;
				if ( std::find_if( entry.m_pRecords.begin( ), entry.m_pRecords.end( ),
					[ & ]( const std::shared_ptr< LagRecord_t >& lag_record ) -> bool {
						return lag_record->m_cAnimData.m_vecVelocity.Length2D( ) > 0.1f; }
				) == entry.m_pRecords.end( ) )
					break;
			}
			
		}
	}


	backup->Apply( entry.m_pPlayer );

	return bestRecord;
}

FORCEINLINE int CRageBot::QuickScan( CBasePlayer* player, LagRecord_t* record, bool& metScaled ) {
	record->Apply( player );

	const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };

	int dmg{ };

	for ( const auto& hb : { HITBOX_STOMACH, HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {// opt
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, record->m_cAnimData.m_pMatrix[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, player, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, point, RagebotAutowall ) };

		if ( static_cast< int >( data.dmg ) < 1 )
			continue;

		if ( hb == HITBOX_HEAD ) {
			if ( data.dmg >= 110 )
				metScaled = true;
		}
		else if ( hb == HITBOX_STOMACH ) {
			auto dmgScaler{ static_cast< float >( ctx.m_pWeaponData->iDamage ) };

			Features::Autowall.ScaleDamage( player, dmgScaler, ctx.m_pWeaponData->flArmorRatio, data.hitgroup );

			if ( data.dmg >= dmgScaler - 10 )
				metScaled = true;
		}

		dmg += data.dmg;
	}
	return dmg;
}