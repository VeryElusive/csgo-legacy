#include "ragebot.h"
#include "exploits.h"
#include "../visuals/visuals.h"
#include "../../utils/performance_monitor.h"
#include "../../utils/ray_tracer.h"

void CRageBot::Main( CUserCmd& cmd, bool shoot ) {
	Reset( );
	{
		CScopedPerformanceMonitor as{ &ctx.m_iRageRecordPerfTimer };
		if ( ctx.m_strDbgLogs.size( ) )
			ctx.m_strDbgLogs.clear( );

		if ( !ctx.m_pWeapon || !ctx.m_pWeaponData )
			return;

		if ( ctx.m_pWeaponData->nWeaponType != m_iLastWeaponType
			|| ctx.m_pWeapon->m_iItemDefinitionIndex( ) != m_iLastWeaponIndex
			|| Menu::Opened ) {
			ParseCfgItems( ctx.m_pWeaponData->nWeaponType );
			SetupHitboxes( );
		}

		if ( !Config::Get<bool>( Vars.RagebotEnable )
			//|| ctx.m_pWeapon->m_bReloading( )
			|| ( ctx.m_pWeaponData->nWeaponType >= WEAPONTYPE_C4
				&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_TASER )
			|| ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER )
			return;

		if ( ctx.m_pWeapon->IsKnife( ) ) {
			if ( Config::Get<bool>( Vars.RagebotKnifebot ) && shoot )
				KnifeBot( cmd );
			return;
		}
		else if ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER && !Config::Get<bool>( Vars.RagebotZeusbot ) )
			return;


		if ( m_iHitboxes.size( ) < 1 )
			return;

		for ( auto i{ 1 }; i <= 64; i++ ) {
			auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
			if ( !player
				|| player->IsDormant( )
				|| player->IsDead( )
				|| player->m_bGunGameImmunity( )
				|| player->IsTeammate( ) )
				continue;

			const auto fov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) ) };
			if ( fov > RagebotFOV )
				continue;

			auto& entry{ Features::AnimSys.m_arrEntries.at( i - 1 ) };
			if ( entry.m_pPlayer != player )
				continue;

			auto target{ std::make_shared< AimTarget_t >( entry ) };

			Features::Ragebot.GetBestLagRecord( entry, target.get( ) );
			if ( !target->m_pRecord )
				continue;

			target->m_pDbgLog = ctx.m_strDbgLogs.emplace_back( std::make_shared< std::string >( "" ) );

			m_cAimTargets.push_back( target );
		}

		ScanTargets( );

		//Features::Misc.AutoStop( cmd );
		//Features::EnginePrediction.RunCommand( cmd );

		if ( shoot )
			Fire( cmd );

		for ( auto& target : m_cAimTargets )
			target->m_pDbgLog->append( " << " + ( std::string )Interfaces::Engine->GetPlayerInfo( target->m_pPlayer->Index( ) )->szName );

		m_cAimTargets.clear( );
		m_pFinalTarget = nullptr;
	}
}

void CRageBot::ScanTargets( ) {
	m_pFinalTarget = PickTarget( );
	if ( !m_pFinalTarget )
		return;

	const auto backup{ std::make_unique< LagBackup_t >( m_pFinalTarget->m_pPlayer ) };

	*m_pFinalTarget->m_pDbgLog = _( "CP" );

	m_pFinalTarget->m_pRecord->Apply( m_pFinalTarget->m_pPlayer );

	if ( !CreatePoints( m_pFinalTarget ) ) {
		backup->Apply( m_pFinalTarget->m_pPlayer );
		return;
	}

	*m_pFinalTarget->m_pDbgLog = _( "SP" );

	for ( auto& point : m_pFinalTarget->m_cPoints )
		ScanPoint( m_pFinalTarget, point );

	backup->Apply( m_pFinalTarget->m_pPlayer );

	m_pFinalTarget->m_cAimPoint = PickPoints( m_pFinalTarget->m_pPlayer, m_pFinalTarget->m_cPoints );

	/*for ( auto& target : m_cAimTargets ) {
		const auto backup{ std::make_unique< LagBackup_t >( target->m_pPlayer ) };

		*target->m_pDbgLog = _( "CP" );

		target->m_pRecord->Apply( target->m_pPlayer, target->m_iResolverSide );
		std::memcpy( target->m_pPlayer->m_CachedBoneData( ).Base( ), target->m_pMatrices[ target->m_iResolverSide ], target->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

		//if ( target->m_pRecord->m_bMultiMatrix )
		//	target->CreateOptimalMatrix( );

		if ( !CreatePoints( target.get( ) ) ) {
			backup->Apply( target->m_pPlayer );
			continue;
		}

		*target->m_pDbgLog = _( "SP" );

		for ( auto& point : target->m_cPoints )
			ScanPoint( target.get( ), point );


		backup->Apply( target->m_pPlayer );

		target->m_cAimPoint = PickPoints( target->m_pPlayer, target->m_cPoints );
	}*/
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

		if ( hb == HITBOX_HEAD )
			ret += 4;
		else
			ret += 2;
	}

	return ret;
}

FORCEINLINE int CRageBot::OffsetDelta( CBasePlayer* player, LagRecord_t* record ) {
	QAngle shootAngle;

	auto eyePos = record->m_cAnimData.m_vecOrigin;
	eyePos.z += player->m_flDuckAmount( ) ? 46.f : 64.f;

	const auto hitboxSet{ ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) ) };
	const auto hitbox = hitboxSet->GetHitbox( HITBOX_CHEST );
	if ( !hitbox )
		return 90;

	const auto point = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] );

	Math::VectorAngles( point - eyePos, shootAngle );

	auto delta = Math::AngleDiff( shootAngle.y, record->m_angEyeAngles.y );

	for ( ; delta > 90; delta -= 180 );
	for ( ; delta < -90; delta += 180 );

	return std::abs( delta );
}

bool CRageBot::CreatePoints( AimTarget_t* target ) {
	const auto hitboxSet = target->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( target->m_pPlayer->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	const auto& missed = target->m_iMissedShots;

	target->m_cPoints.reserve( CalcPointCount( hitboxSet ) );

	int mindmg{ std::max( RagebotPenetrationDamage, RagebotMinimumDamage ) };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = RagebotOverrideDamage;
	else if ( RagebotScaleDamage )
		mindmg *= target->m_pPlayer->m_iHealth( ) / 100.f;

	for ( const auto& hb : m_iHitboxes ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		if ( ( Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).enabled 
			|| ( Config::Get<bool>( Vars.RagebotForceBaimAfterX ) && Config::Get<int>( Vars.RagebotForceBaimAfterXINT ) < target->m_iMissedShots ) )
			&& hb == HITBOX_HEAD )
			continue;

		// ignore limbs when moving
		if ( RagebotIgnoreLimbs && ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 5.f
			&& ( hitbox->iGroup == HITGROUP_LEFTLEG || hitbox->iGroup == HITGROUP_RIGHTLEG
				|| hitbox->iGroup == HITGROUP_LEFTARM || hitbox->iGroup == HITGROUP_RIGHTARM ) )
			continue;

		auto damage{ static_cast< float >( ctx.m_pWeaponData->iDamage ) };
		Features::Autowall.ScaleDamage( target->m_pPlayer, damage, ctx.m_pWeaponData->flArmorRatio, hitbox->iGroup, 4.f );
		// impossible
		if ( damage < mindmg )
			continue;

		int scale = ( hb ? RagebotBodyScale : RagebotHeadScale );

		if ( !RagebotStaticPointscale ) {
			// dynamic multipoints:
			// check the side delta + inac
			scale = 100;

			// this will take us down to a maximum 35 scale
			scale -= std::floor( std::min( Features::EnginePrediction.Inaccuracy + Features::EnginePrediction.Spread, 0.15f ) * 333.333f );

			// now compare side delta
			scale -= std::min( 90 - std::abs( 90 - OffsetDelta( target->m_pPlayer, target->m_pRecord.get( ) ) ), scale );
		}

		float scaleFloat{ scale / 100.f };

		auto& matrix{ target->m_pRecord->m_pMatrix[ hitbox->iBone ] };

		Vector center{ ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f };
		center = Math::VectorTransform( center, matrix );

		// dont add the center point in here because we are ordering them
		//target->m_cPoints.emplace_back( center, hitbox->iGroup, center );

		if ( IsMultiPointEnabled( hb ) )
			Multipoint( center, matrix, target->m_cPoints, hitbox, hitboxSet, scaleFloat, hb );
		else
			target->m_cPoints.emplace_back( center, hitbox->iGroup );

		/*for ( auto& point : target->m_cPoints ) {
			Interfaces::DebugOverlay->AddBoxOverlay( point.m_vecPoint, Vector( -0.1, -0.1, -0.1 ), Vector( 0.1, 0.1, 0.1 ), QAngle( 0.f, -0.f, 0.f ),
				255, 255, 255, 0, 1 );
		}*/
	}

	return !target->m_cPoints.empty( );
}

void CRageBot::Multipoint( Vector& center, matrix3x4_t& matrix, std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float scale, int index ) {
	if ( hitbox->flRadius == -1.f ) {
		aimPoints.emplace_back( center, hitbox->iGroup );

		auto d = ( hitbox->vecBBMin.x - center.x ) * scale;
		aimPoints.emplace_back( Math::VectorTransform( { center.x + d, center.y, center.z }, matrix ),
			hitbox->iGroup );

		auto d0 = ( hitbox->vecBBMax.x - center.x ) * scale;
		aimPoints.emplace_back( Math::VectorTransform( { center.x + d0, center.y, center.z }, matrix ),
			hitbox->iGroup );

		return;
	}

	const auto hitbox_min{ Math::VectorTransform( hitbox->vecBBMin, matrix ) };
	const auto hitbox_max{ Math::VectorTransform( hitbox->vecBBMax, matrix ) };

	auto forward_dir = ( center - ctx.m_vecEyePos ).Normalized( );
	auto cylinder_dir = ( hitbox_max - hitbox_min ).Normalized( );

	auto cross = cylinder_dir.CrossProduct( forward_dir );
	QAngle forward_ang;
	Math::VectorAngles( forward_dir, forward_ang );

	Vector right, up;

	if ( index == HITBOX_HEAD ) {
		QAngle cross_ang;
		Math::VectorAngles( cross, cross_ang );
		Math::AngleVectors( cross_ang, nullptr, &right, &up );
		cross_ang.z = forward_ang.x;

		auto tmp{ cross };
		cross = right;
		right = tmp;
	}
	else
		Math::VectorVectors( forward_dir, up, right );

	RayTracer::Hitbox box( hitbox_min, hitbox_max, hitbox->flRadius );
	RayTracer::Trace trace;

	if ( index == HITBOX_HEAD ) {
		Vector middle = ( right.Normalized( ) + up.Normalized( ) ) * 0.5f;
		Vector middle0 = ( right.Normalized( ) - up.Normalized( ) ) * 0.5f;

		aimPoints.emplace_back( center, hitbox->iGroup );

		// left
		RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( middle * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );

		// right
		ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( middle0 * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );

		// bottom
		ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );

		// top
		ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );
	}
	else {
		// left
		RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );

		// right
		ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
		RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );
		aimPoints.emplace_back( center + ( trace.m_traceEnd - center ) * scale, hitbox->iGroup );

		aimPoints.emplace_back( center, hitbox->iGroup );
	}
}

int ClipRayToHitbox( const Ray_t& ray, mstudiobbox_t* box, matrix3x4_t& mat, CGameTrace& tr ) {
	int retval = -1;

	__asm {
		mov ecx, ray
		mov edx, box
		push tr
		push mat
		call Offsets::Sigs.ClipRayToHitbox
		mov retval, eax
		add esp, 8
	}

	return retval;
}

bool intersectHitbox( Vector end, mstudiobbox_t* hitbox, matrix3x4_t* matrix ) {
	matrix3x4_t transform;
	transform.SetAngles( hitbox->angOffsetOrientation.y, hitbox->angOffsetOrientation.x, hitbox->angOffsetOrientation.y );
	auto hitbox_matrix = matrix[ hitbox->iBone ] * ( transform );

	Trace_t tr;
	tr.flFraction = 1.f;
	tr.bStartSolid = false;

	return ClipRayToHitbox( { ctx.m_vecEyePos, end }, hitbox, hitbox_matrix, tr ) > -1;
}

void CRageBot::ScanPoint( AimTarget_t* target, AimPoint_t& point ) {
	//if ( point.m_bScanned )
	//	return;

	//point.m_bScanned = true;

	const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, target->m_pPlayer, ctx.m_pWeaponData,
		ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
		ctx.m_vecEyePos, point.m_vecPoint, RagebotAutowall ) };

	if ( static_cast< int >( data.dmg ) < 1 )
		return;

	point.m_flDamage = static_cast< int >( data.dmg );
	point.m_iHitgroup = data.hitgroup;
	point.m_iHitbox = data.hitbox;// TODO: MAKE ENTIRE RAGEBOT RUN OFF HITBOX

	//if ( data.hitbox != HITBOX_HEAD && point.m_iHitbox == HITBOX_HEAD )
	//	return;

	point.m_bPenetrated = data.penetrationCount < 4;

	int mindmg{ point.m_bPenetrated ? RagebotPenetrationDamage : RagebotMinimumDamage };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = RagebotOverrideDamage;
	else if ( RagebotScaleDamage )
		mindmg *= target->m_pPlayer->m_iHealth( ) / 100.f;

	if ( point.m_flDamage < mindmg )
		return;

	*target->m_pDbgLog = _( "SA" );

	m_bShouldStop = true;

	if ( std::find( m_iHitboxes.begin( ), m_iHitboxes.end( ), data.hitbox ) == m_iHitboxes.end( ) )
		return;
	
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
			const bool isDTEnabled{ ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };
			if ( RagebotPreferBaimDoubletap && ( isDTEnabled && ctx.m_iTicksAllowed ) )
				break;
		}

		// points are ordered in the best place to shoot at
		if ( point.m_flDamage >= bestPoint->m_flDamage )
			bestPoint = &point;

		/*if ( point.m_iDesyncIntersections >= bestPoint->m_iDesyncIntersections ) {
			if ( point.m_iCBBIntersections >= bestPoint->m_iCBBIntersections
				|| point.m_iDesyncIntersections > bestPoint->m_iDesyncIntersections ) {
				if ( point.m_flDamage >= bestPoint->m_flDamage )
					bestPoint = &point;
			}
		}*/
	}

	return bestPoint;
}

// sry
QAngle MatrixGetAngles( matrix3x4_t mat ) {
	constexpr auto ONE_RADIAN_IN_DEGREES = 180.f / static_cast< float >( M_PI );
	auto rads2degs = [ ]( float rads ) { return rads * ONE_RADIAN_IN_DEGREES; };

	Vector fwd{ mat[ 0 ][ 0 ], mat[ 1 ][ 0 ], mat[ 2 ][ 0 ] }, left{ mat[ 0 ][ 1 ], mat[ 1 ][ 1 ], mat[ 2 ][ 1 ] };

	float xy_dist = fwd.Length2D( );
	return xy_dist > 0.001f ? QAngle{ rads2degs( std::atan2( -fwd.z, xy_dist ) ), rads2degs( std::atan2( fwd.y, fwd.x ) ), rads2degs( std::atan2( left.z, mat[ 2 ][ 2 ] ) ) } :
		QAngle{ rads2degs( std::atan2( -fwd.z, xy_dist ) ), rads2degs( std::atan2( -left.x, left.y ) ), 0.f };
}

void CRageBot::Fire( CUserCmd& cmd ) {
	if ( !ctx.m_bCanShoot )
		return;

	if ( !m_pFinalTarget || !m_pFinalTarget->m_cAimPoint )
		return;

	*m_pFinalTarget->m_pDbgLog = _( "HC" );

	const auto backup{ std::make_unique< LagBackup_t >( m_pFinalTarget->m_pPlayer ) };

	m_pFinalTarget->m_pRecord->Apply( m_pFinalTarget->m_pPlayer );

	QAngle angle{ };
	Math::VectorAngles( m_pFinalTarget->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( angle.y, angle.x );

	Math::VectorAngles( m_pFinalTarget->m_cAimPoint->m_vecPoint - ctx.m_vecEyePos, angle );

	const int hitchance{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 : 
		ctx.m_pLocal->m_bIsScoped( ) || !ctx.m_pWeaponData || ctx.m_pWeaponData->nWeaponType != WEAPONTYPE_SNIPER 
		? RagebotHitchance : RagebotNoscopeHitchance };

	if ( HitChance( m_pFinalTarget->m_pPlayer, angle, hitchance, m_pFinalTarget->m_cAimPoint->m_iHitbox ) ) {
		if ( RagebotAutoFire )
			cmd.iButtons |= IN_ATTACK;

		if ( cmd.iButtons & IN_ATTACK ) {
			// IN_ATTACK2 and IN_USE is checked before IN_ATTACK and will CANCEL THE FUUCKING ATTACK...
			cmd.iButtons &= ~IN_ATTACK2;
			cmd.iButtons &= ~IN_USE;

			m_bShouldStop = false;
			const auto backupView{ cmd.viewAngles };
			cmd.viewAngles = angle - ctx.m_pLocal->m_aimPunchAngle( ) * Offsets::Cvars.weapon_recoil_scale->GetFloat( );

			Features::Misc.MoveMINTFix( cmd, backupView, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );

			cmd.iTickCount = TIME_TO_TICKS( m_pFinalTarget->m_pRecord->m_cAnimData.m_flSimulationTime + ctx.m_flLerpTime );

			//backup->m_flNewBoundsMaxs = m_pFinalTarget->m_pPlayer->m_flNewBoundsMaxs( );
			//backup->m_flNewBoundsTime = m_pFinalTarget->m_pPlayer->m_flNewBoundsTime( );

			auto& entry{ Features::AnimSys.m_arrEntries.at( m_pFinalTarget->m_pPlayer->Index( ) - 1 ) };

			if ( !RagebotSilentAim )
				Interfaces::Engine->SetViewAngles( angle );

			int delay{ };
			if ( ctx.m_bFakeDucking )
				delay = 15 - Interfaces::ClientState->nChokedCommands;

			Features::Shots.AddShot( m_pFinalTarget->m_pPlayer, m_pFinalTarget->m_pRecord, 
				m_pFinalTarget->m_cAimPoint->m_iHitgroup, ctx.m_vecEyePos, m_pFinalTarget->m_cAimPoint->m_vecPoint );

			if ( Config::Get<bool>( Vars.MiscHitMatrix ) ) {
				if ( Config::Get<int>( Vars.MiscShotVisualizationType ) == 0 )
					Features::Visuals.Chams.AddHitmatrix( m_pFinalTarget->m_pPlayer, m_pFinalTarget->m_pPlayer->m_CachedBoneData( ).Base( ) );
				else {
					auto hdr{ Interfaces::ModelInfo->GetStudioModel( m_pFinalTarget->m_pPlayer->GetModel( ) ) };
					if ( hdr ) {
						auto matrix = m_pFinalTarget->m_pRecord->m_pMatrix;
						auto hitboxSet = hdr->GetHitboxSet( m_pFinalTarget->m_pPlayer->m_nHitboxSet( ) );
						if ( hitboxSet ) {
							for ( int i{ }; i < hitboxSet->nHitboxes; ++i ) {
								const auto hitbox = hitboxSet->GetHitbox( i );
								const auto& color{ Config::Get<Color>( Vars.MiscHitMatrixCol ) };

								if ( hitbox->flRadius <= 0.f ) {
									matrix3x4_t transform{ };
									transform.SetAngles( hitbox->angOffsetOrientation.y, hitbox->angOffsetOrientation.x, hitbox->angOffsetOrientation.z );
									if ( Config::Get<bool>( Vars.MiscHitMatrixXQZ ) )
										Interfaces::DebugOverlay->AddBoxOverlay2( matrix[ hitbox->iBone ].GetOrigin( ), hitbox->vecBBMin, hitbox->vecBBMax, MatrixGetAngles( matrix[ hitbox->iBone ] * transform ), { 0, 0, 0, 0 }, color, Config::Get<float>( Vars.MiscHitMatrixTime ) );
									else
										Interfaces::DebugOverlay->AddBoxOverlay( matrix[ hitbox->iBone ].GetOrigin( ), hitbox->vecBBMin, hitbox->vecBBMax, MatrixGetAngles( matrix[ hitbox->iBone ] * transform ), color[ 0 ], color[ 1 ], color[ 2 ], 0, Config::Get<float>( Vars.MiscHitMatrixTime ) );
									continue;
								}

								const auto min = Math::VectorTransform( hitbox->vecBBMin, matrix[ hitbox->iBone ] );
								const auto max = Math::VectorTransform( hitbox->vecBBMax, matrix[ hitbox->iBone ] );

								Interfaces::DebugOverlay->AddCapsuleOverlay( min, max, hitbox->flRadius, color[ 0 ], color[ 1 ], color[ 2 ], color[ 3 ], Config::Get<float>( Vars.MiscHitMatrixTime ), 0, Config::Get<bool>( Vars.MiscHitMatrixXQZ ) );
							}
						}
					}
				}
			}

			const std::string message =
				_( "shot " ) + ( std::string ) Interfaces::Engine->GetPlayerInfo( m_pFinalTarget->m_pPlayer->Index( ) )->szName +
				_( " | hitgroup: " ) + Hitgroup2Str( m_pFinalTarget->m_cAimPoint->m_iHitgroup ) +
				_( " | pred damage: " ) + std::to_string( static_cast< int >( m_pFinalTarget->m_cAimPoint->m_flDamage ) ).c_str( ) +
				_( " | backtrack: " ) + std::to_string( Interfaces::ClientState->iServerTick - m_pFinalTarget->m_pRecord->m_iReceiveTick ) + _( " ticks" ) +
				//_( " | extrapolated: " ) + std::to_string( m_pFinalTarget->m_bExtrapolating ) +
				//_( " | optimal matrix: " ) + std::to_string( m_pFinalTarget->m_bUsingOptimalMatrix ) +
				_( " | resolved: " ) + std::to_string( m_pFinalTarget->m_pRecord->m_bResolverThisTick ) +
				_( " | missed: " ) + std::to_string( m_pFinalTarget->m_iMissedShots ) +
				_( " | validity: " ) + std::to_string( m_pFinalTarget->m_pRecord->Validity( ) )
				/*_( " | host_currentframetick: " ) + std::to_string( **( int** )Offsets::Sigs.host_currentframetick ) +
				_( " | numticks: " ) + std::to_string( **( int** )Offsets::Sigs.numticks ) +
				_( " | ping ticks: " ) + std::to_string( TIME_TO_TICKS( ctx.m_flRealOutLatency + ctx.m_flInLatency ) ) +
				_( " | cmd num: " ) + std::to_string( cmd.iCommandNumber )*/;

			Features::Logger.Log( message, false );
		}
	}
	else if ( RagebotAutoScope && !ctx.m_pWeapon->m_zoomLevel( )
		&& ctx.m_pWeaponData && ctx.m_pWeaponData->nWeaponType == WEAPONTYPE_SNIPER )
		cmd.iButtons |= IN_ATTACK2;

	backup->Apply( m_pFinalTarget->m_pPlayer );
}

bool CRageBot::HitChance( CBasePlayer* player, const QAngle& ang, int hitchance, int index ) {
	const auto hitsNeeded{ static_cast< int >( 256.f * ( hitchance / 100.f ) ) };
	int totalHits{ };

	Vector fwd{ }, right{ }, up{ };
	Math::AngleVectors( ang, &fwd, &right, &up );

	const auto item_index = ctx.m_pWeapon->m_iItemDefinitionIndex( );
	const auto recoil_index = ctx.m_pWeapon->m_flRecoilIndex( );

	for ( int i{ }; i < 256; ++i ) {
		const auto spreadAngle{ CalcSpreadAngle( item_index, ctx.m_pWeaponData->iBullets, recoil_index, i ) };

		const auto dir{ ( fwd + ( right * spreadAngle.x ) + ( up * spreadAngle.y ) ).Normalized( ) };

		const auto end{ ctx.m_vecEyePos + dir * ctx.m_pWeaponData->flRange };

		Trace_t tr;
		Interfaces::EngineTrace->ClipRayToEntity( { ctx.m_vecEyePos, end }, MASK_SHOT_PLAYER, player, &tr );

		if ( tr.pHitEntity == player ) {
			if ( !RagebotHitchanceThorough
				|| tr.iHitbox == index )
				totalHits++;
		}

		if ( totalHits >= hitsNeeded )
			return true;

		// no chance, sorry bud
		if ( 256 - i < hitsNeeded - totalHits ) {
			ctx.m_strDbgLogs.emplace_back( std::make_shared< std::string >( std::to_string( totalHits ) ) );
			return false;
		}
	}

	return false;
}

Vector2D CRageBot::CalcSpreadAngle( const int item_index, const int bullets, const float recoil_index, const int i ) {
	Math::RandomSeed( i );

	auto v1 = Math::RandomFloat( 0.0f, 1.0f );
	auto v2 = Math::RandomFloat( 0.0f, M_2PI );

	float v3{ }, v4{ };

	using fn_t = void( __stdcall* )( int, int, int, float*, float* );
	if ( Offsets::Cvars.weapon_accuracy_shotgun_spread_patterns->GetInt( ) > 0 )
		reinterpret_cast< fn_t >( Offsets::Sigs.CalcShotgunSpread )( item_index, 0, static_cast< int >( bullets * recoil_index ), &v4, &v3 );
	else {
		v3 = Math::RandomFloat( 0.0f, 1.0f );
		v4 = Math::RandomFloat( 0.0f, M_2PI );
	}

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
		//if ( !target->m_cAimPoint )
		//	continue;

		*target->m_pDbgLog = _( "PT" );

		if ( !retTarget ) {
			retTarget = target.get( );
			continue;
		}

		switch ( Config::Get<int>( Vars.RagebotTargetSelection ) )
		{
		case 0: {// highest damage
			if ( target->m_iBestDamage > retTarget->m_iBestDamage )
				retTarget = target.get( );
		}break;
		case 1: // fov
		{
			const auto firstFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, target->m_pPlayer->GetAbsOrigin( ) ) ) };
			const auto secondFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, retTarget->m_pPlayer->GetAbsOrigin( ) ) ) };
			if ( firstFov < secondFov )
				retTarget = target.get( );

			break;
		}
		case 2: // lowest distance
		{
			const float delta1{ target->m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };
			const float delta2{ retTarget->m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };

			if ( delta1 < delta2 )
				retTarget = target.get( );

			break;
		}
		case 3: // lowest health
		{
			if ( target->m_pPlayer->m_iHealth( ) < retTarget->m_pPlayer->m_iHealth( ) )
				retTarget = target.get( );

			break;
		}
		}
	}

	return retTarget;
}

bool ExtrapolateBreakLC( PlayerEntry& entry ) {
	const auto& record{ entry.m_pRecords.back( ) };
	const auto extrapolatedOrigin{ entry.m_optPreviousData->m_vecOrigin +
		( entry.m_optPreviousData->m_vecVelocity 
			* TICKS_TO_TIME( std::min( Interfaces::Globals->iTickCount - entry.m_iLastRecievedTick + TIME_TO_TICKS( ctx.m_flRealOutLatency ), 16 ) ) ) };

	return ( extrapolatedOrigin - record->m_cAnimData.m_vecOrigin ).LengthSqr( ) > 4096.f;
}

// rework this
void CRageBot::GetBestLagRecord( PlayerEntry& entry, AimTarget_t* target ) {
	if ( entry.m_pRecords.empty( ) )
		return;

	std::shared_ptr< LagRecord_t > bestRecord{ };

	const auto backup{ std::make_unique< LagBackup_t >( entry.m_pPlayer ) };

	//const auto backupGE{ target->m_pPlayer->m_hGroundEntity( ) };

	int delay{ };
	if ( ctx.m_bFakeDucking )
		delay = 15 - Interfaces::ClientState->nChokedCommands;

	std::vector <int> hitgroups{ HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT };//HITBOX_STOMACH

	if ( const auto& record{ entry.m_pRecords.back( ) }; record->m_bBrokeLC  /* || ExtrapolateBreakLC( entry )*/ ) {
		if ( TIME_TO_TICKS( ctx.m_flRealOutLatency ) + Interfaces::Globals->iTickCount - record->m_iReceiveTick >= record->m_iNewCmds )
			return;

		target->m_pRecord = record;

		bool a{ };
		int b{ };
		const auto dmg{ QuickScan( target, hitgroups ) };
		//entry.m_pPlayer->m_hGroundEntity( ) = backupGE;

		if ( dmg < 1 )
			target->m_pRecord = nullptr;

		target->m_iBestDamage = dmg;
		backup->Apply( entry.m_pPlayer );
		return;
	}

	int bestSafepoints{ };
	//float bestHeadDifference{ FLT_MAX };

	bool first{ };
	const auto& idx{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) };

	std::shared_ptr< LagRecord_t > oldestRecord{ };
	uint8_t bestValidity{ };

	for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };

		if ( record->m_bBrokeLC )
			break;

		const auto validity{ record->Validity( ) };
		if ( !validity )
			continue;

		if ( validity < 3
			&& ctx.m_iTicksAllowed )
			continue;

		oldestRecord = record;
	}

	if ( oldestRecord ) {
		target->m_pRecord = oldestRecord;

		int safepoints{ };
		const auto dmg{ QuickScan( target, hitgroups ) };
		if ( dmg >= 1 ) {
			//const auto headDifference{ ( target->m_pMatrices[ target->m_iResolverSide ][ 8 ].GetOrigin( )
			//	- oldestRecord->m_cAnimData.m_arrSides.at( target->m_iResolverSide ).m_pMatrix[ 8 ].GetOrigin( )
			//	).Length( ) };

			bestSafepoints = safepoints;
			//bestHeadDifference = headDifference;
			bestRecord = oldestRecord;
			target->m_iBestDamage = dmg;
		}
	}

	for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };

		if ( record == oldestRecord )
			break;

		if ( record->m_bBrokeLC )
			break;

		const auto validity{ record->Validity( ) };
		if ( !validity )
			continue;

		if ( validity < 3
			&& ctx.m_iTicksAllowed )
			continue;

		//if ( !bestRecord && record->m_cAnimData.m_vecOrigin == oldestRecord->m_cAnimData.m_vecOrigin )
		//	break;

		target->m_pRecord = record;
		int safepoints{ };
		const auto dmg{ QuickScan( target, hitgroups ) };
		if ( !first ) {
			// maybe both stomach + head?
			hitgroups = { ( idx == WEAPON_SCAR20 || idx == WEAPON_G3SG1 || idx == WEAPON_AWP )
				? HITBOX_STOMACH : HITBOX_HEAD };

			first = true;
		}

		if ( dmg < 1 ) {
			if ( !bestRecord )
				break;

			continue;
		}

		if ( Config::Get<keybind_t>( Vars.RagebotForceSafePointKey ).enabled && !record->m_bResolverThisTick )
			continue;

		//const auto headDifference{ ( target->m_pMatrices[ target->m_iResolverSide ][ 8 ].GetOrigin( )
		//	- record->m_cAnimData.m_arrSides.at( target->m_iResolverSide ).m_pMatrix[ 8 ].GetOrigin( )
		//	).Length( ) };

		if ( target->m_iBestDamage > dmg
			|| dmg - target->m_iBestDamage <= 10 ) {

			if ( dmg >= target->m_iBestDamage + 10 )
				bestRecord = record;
			else if ( record->m_bResolverThisTick )
				bestRecord = record;

			if ( dmg > target->m_iBestDamage )
				target->m_iBestDamage = dmg;
		}
	}
	
	//const auto futureAmount{ TIME_TO_TICKS( ctx.m_flRealOutLatency ) + ( Interfaces::Globals->iTickCount - entry.m_iLastRecievedTick ) };//+ 2 ?

	if ( bestRecord ) {
		if ( Config::Get<keybind_t>( Vars.RagebotForceSafePointKey ).enabled && !target->m_pRecord->m_bResolverThisTick )
			target->m_pRecord = nullptr;
		else
			target->m_pRecord = bestRecord;
	}
	else
		target->m_pRecord = nullptr;

	//entry.m_pPlayer->m_hGroundEntity( ) = backupGE;
	backup->Apply( entry.m_pPlayer );
}

FORCEINLINE int CRageBot::QuickScan( AimTarget_t* target, std::vector<int> hitgroups ) {
	target->m_pRecord->Apply( target->m_pPlayer );

	std::memcpy( target->m_pPlayer->m_CachedBoneData( ).Base( ), target->m_pRecord->m_pMatrix, target->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto hitboxSet{ target->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( target->m_pPlayer->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return 0;

	int dmg{ };

	for ( const auto& hb : hitgroups ) {// opt
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, target->m_pRecord->m_pMatrix[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, target->m_pPlayer, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, point, RagebotAutowall ) };

		if ( static_cast< int >( data.dmg ) < 1 )
			continue;

		if ( data.dmg > dmg )
			dmg = data.dmg;
	}

	if ( dmg >= 0 ) {
		const auto head{ hitboxSet->GetHitbox( HITBOX_HEAD ) };

		const auto point{ Math::VectorTransform( ( head->vecBBMin + head->vecBBMax ) / 2.f, target->m_pRecord->m_pMatrix[ head->iBone ] ) };
	}

	return dmg;
}