#include "ragebot.h"
#include "../../utils/ray_tracer.h"
#include "../visuals/visuals.h"

#define THIS Features::Ragebot
#define RECORD_MATRIX( record ) record->m_cAnimData.m_arrSides.at( 0 ).m_pMatrix

void CRageBot::Main( CUserCmd& cmd, bool shoot ) {
	m_bShouldStop = false;

	if ( m_vecPrecomputedSeeds.empty( ) )
		PrecomputeSeeds( );

	if ( ctx.m_strDbgLogs.size( ) )
		ctx.m_strDbgLogs.clear( );

	if ( !ctx.m_pWeapon || !ctx.m_pWeaponData )
		return;

	if ( !Config::Get<bool>( Vars.RagebotEnable )
		//|| ctx.m_pWeapon->m_bReloading( )
		|| ( ctx.m_pWeaponData->nWeaponType >= WEAPONTYPE_C4
			&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_TASER )
		|| ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER )
		return;

	ParseCfgItems( ctx.m_pWeaponData->nWeaponType );

	if ( ctx.m_pWeapon->IsKnife( ) ) {
		if ( Config::Get<bool>( Vars.RagebotKnifebot ) && shoot )
			KnifeBot( cmd );
		return;
	}
	else if ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER && !Config::Get<bool>( Vars.RagebotZeusbot ) )
		return;

	if ( m_vecHitboxes.empty( ) )
		return;

	auto bestTarget{ new AimTarget_t };
	auto target{ new AimTarget_t };

	for ( auto i{ 1 }; i < 64; i++ ) {
		auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player
			|| player->IsDormant( )
			|| player->IsDead( )
			|| player->m_bGunGameImmunity( )
			|| player->IsTeammate( ) )
			continue;

		const auto fov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) ) };
		if ( fov > MenuVars.RagebotFOV )
			continue;

		auto& entry{ Features::AnimSys.m_arrEntries.at( i - 1 ) };
		if ( entry.m_pPlayer != player
			|| entry.m_pRecords.empty( ) ) {
			ctx.m_strDbgLogs.emplace_back( std::make_shared< std::string >( "ZE << " + ( std::string ) Interfaces::Engine->GetPlayerInfo( player->Index( ) )->szName ) );
			continue;
		}

		const auto name{ static_cast< std::string >( Interfaces::Engine->GetPlayerInfo( player->Index( ) )->szName ) };

		target->m_pDbgLog = nullptr;
		target->m_pPlayer = player;

		target->GetBestLagRecord( entry );

		if ( !target->m_pRecord ) {
			if ( target->m_pDbgLog )
				target->m_pDbgLog->append( " << " + ( std::string ) Interfaces::Engine->GetPlayerInfo( player->Index( ) )->szName );

			continue;
		}

		bestTarget->CompareTarget( *target );
	}

	delete target;

	if ( !bestTarget->m_pPlayer ) {
		delete bestTarget;
		return;
	}

	if ( !bestTarget->m_pDbgLog )
		bestTarget->m_pDbgLog = ctx.m_strDbgLogs.emplace_back( std::make_shared< std::string >( "" ) );

	*bestTarget->m_pDbgLog = _( "ST" );

	bestTarget->ScanTarget( m_vecHitboxes );

	if ( bestTarget->m_cPoint.m_bValid
		&& shoot ) {
		*bestTarget->m_pDbgLog = _( "HC" );

		bestTarget->Fire( cmd );
	}

	bestTarget->m_pDbgLog->append( " << " + ( std::string ) Interfaces::Engine->GetPlayerInfo( bestTarget->m_pPlayer->Index( ) )->szName );

	delete bestTarget;
}

Vector2D CRageBot::CalcSpreadAngle( const int itemIndex, const float recoilIndex, float one, float twoPI ) {

	auto radiusCurveDensity{ one }, spreadCurveDensity{ radiusCurveDensity };
	auto theta0{ twoPI }, theta1{ theta0 };

	// TODO: add shotgun shit.

	if ( itemIndex == WEAPON_REVOLVER )
		radiusCurveDensity = 1.f - radiusCurveDensity * radiusCurveDensity, spreadCurveDensity = 1.f - spreadCurveDensity * spreadCurveDensity;
	else if ( itemIndex == WEAPON_NEGEV && recoilIndex < 3.f ) {
		for ( int j = 3; j > recoilIndex; --j )
			radiusCurveDensity *= radiusCurveDensity, spreadCurveDensity *= spreadCurveDensity;

		radiusCurveDensity = 1.f - radiusCurveDensity, spreadCurveDensity = 1.f - spreadCurveDensity;
	}

	return { radiusCurveDensity * Features::EnginePrediction.Inaccuracy * std::cos( theta0 ) + spreadCurveDensity * Features::EnginePrediction.Spread * std::cos( theta1 ),
			 radiusCurveDensity * Features::EnginePrediction.Inaccuracy * std::sin( theta0 ) + spreadCurveDensity * Features::EnginePrediction.Spread * std::sin( theta1 ) };
}

int AimTarget_t::HitChance( const QAngle& ang, int hitchance ) {
	Vector fwd{ }, right{ }, up{ };
	Math::AngleVectors( ang, &fwd, &right, &up );

	int total_hits = 0;

	for ( auto& seed : THIS.m_vecPrecomputedSeeds ) {
		const auto spread{ THIS.CalcSpreadAngle( ctx.m_pWeapon->m_iItemDefinitionIndex( ), ctx.m_pWeapon->m_flRecoilIndex( ), seed.first, seed.second ) };

		const auto dir{ ( fwd + right * spread.x + up * spread.y ).Normalized( ) };

		Trace_t tr;
		Interfaces::EngineTrace->ClipRayToEntity( { ctx.m_vecEyePos, ctx.m_vecEyePos + dir * ctx.m_pWeaponData->flRange }, MASK_SHOT_HULL | CONTENTS_HITBOX, this->m_pPlayer, &tr );

		if ( tr.pHitEntity == this->m_pPlayer ) {
			if ( !THIS.MenuVars.RagebotHitchanceThorough
				|| tr.iHitGroup == this->m_cPoint.m_iHitgroup )
				++total_hits;
		}
	}

	return static_cast< int >( total_hits / 256.f * 100.f );
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

void AimTarget_t::Fire( CUserCmd& cmd ) {
	if ( !ctx.m_bCanShoot )
		return;

	const auto backup{ new LagBackup_t( this->m_pPlayer ) };

	this->m_pRecord->Apply( this->m_pPlayer );

	std::memcpy( this->m_pPlayer->m_CachedBoneData( ).Base( ), RECORD_MATRIX( this->m_pRecord ), this->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	QAngle angle{ };
	Math::VectorAngles( this->m_cPoint.m_vecPoint - ctx.m_vecEyePos, angle );

	const auto state{ ctx.m_pLocal->m_pAnimState( ) };
	if ( state && state->m_pPlayer && ( state->m_bLanding || state->flDuckAmount || ctx.m_pLocal->m_hGroundEntity( ) == -1 ) ) {
		ctx.m_vecEyePos = ctx.m_pLocal->GetEyePosition( angle.y, angle.x );

		Math::VectorAngles( this->m_cPoint.m_vecPoint - ctx.m_vecEyePos, angle );
	}

	const int hitchance{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER ? 80 :
		ctx.m_pLocal->m_bIsScoped( ) || !ctx.m_pWeaponData || ctx.m_pWeaponData->nWeaponType != WEAPONTYPE_SNIPER
		? THIS.MenuVars.RagebotHitchance : THIS.MenuVars.RagebotNoscopeHitchance };

	const auto chance{ HitChance( angle, hitchance ) };

	if ( chance >= hitchance ) {
		if ( THIS.MenuVars.RagebotAutoFire )
			cmd.iButtons |= IN_ATTACK;

		if ( cmd.iButtons & IN_ATTACK ) {
			cmd.iButtons &= ~IN_ATTACK2;
			cmd.iButtons &= ~IN_USE;

			cmd.viewAngles = angle - ctx.m_pLocal->m_aimPunchAngle( ) * Displacement::Cvars.weapon_recoil_scale->GetFloat( );

			if ( m_bAbuseRecord )
				cmd.iTickCount = 0;
			else
				cmd.iTickCount = TIME_TO_TICKS( this->m_pRecord->m_cAnimData.m_flSimulationTime + ctx.m_flLerpTime );

			if ( !THIS.MenuVars.RagebotSilentAim )
				Interfaces::Engine->SetViewAngles( angle );

			auto& entry{ Features::AnimSys.m_arrEntries.at( this->m_pPlayer->Index( ) - 1 ) };

			Features::Shots.AddShot( this->m_pPlayer, this->m_pRecord,
				this->m_cPoint.m_iHitgroup, ctx.m_vecEyePos, this->m_cPoint.m_vecPoint, this->m_bExtrapolating );

			if ( Config::Get<bool>( Vars.MiscHitMatrix ) ) {
				if ( Config::Get<int>( Vars.MiscShotVisualizationType ) == 0 )
					Features::Visuals.Chams.AddHitmatrix( this->m_pPlayer, this->m_pPlayer->m_CachedBoneData( ).Base( ) );
				else {
					auto hdr{ Interfaces::ModelInfo->GetStudioModel( this->m_pPlayer->GetModel( ) ) };
					if ( hdr ) {
						auto matrix = RECORD_MATRIX( this->m_pRecord );
						auto hitboxSet = hdr->GetHitboxSet( this->m_pPlayer->m_nHitboxSet( ) );
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

			const std::string message{
				_( "shot " ) + ( std::string ) Interfaces::Engine->GetPlayerInfo( this->m_pPlayer->Index( ) )->szName +
				_( " | hitgroup: " ) + THIS.HitgroupToString( this->m_cPoint.m_iHitgroup ) +
				_( " | pred damage: " ) + std::to_string( static_cast< int >( this->m_cPoint.m_flDamage ) ).c_str( ) +
				_( " | hitchance: " ) + std::to_string( chance ).c_str( ) +
				_( " | backtrack: " ) + std::to_string( Interfaces::ClientState->iServerTick - this->m_pRecord->m_iReceiveTick ) + _( " ticks" ) +
				_( " | extrapolated: " ) + std::to_string( this->m_bExtrapolating ) +
				_( " | moving: " ) + std::to_string( this->m_pRecord->m_cAnimData.m_pLayers[ 6 ].flPlaybackRate != 0.f ) +
				_( " | AB: " ) + std::to_string( m_bAbuseRecord ) +
				_( " | lby update: " ) + std::to_string( this->m_pRecord->m_bLBYUpdate ) +
				_( " | validity: " ) + std::to_string( this->m_pRecord->Validity( ) )};

			Features::Logger.Log( message, false );
		}
	}
	else {
		*this->m_pDbgLog = _( "HC[" ) + std::to_string( chance ) + "]";

		if ( THIS.MenuVars.RagebotAutoScope && !ctx.m_pWeapon->m_zoomLevel( )
			&& ctx.m_pWeaponData && ctx.m_pWeaponData->nWeaponType == WEAPONTYPE_SNIPER )
			cmd.iButtons |= IN_ATTACK2;
	}

	backup->Apply( this->m_pPlayer );
	delete backup;

}

void AimTarget_t::CompareTarget( AimTarget_t& target ) {
	if ( !this->m_pPlayer ) {
		//const auto backupM{ this->m_pMatrices };
		*this = target;
		//this->m_pMatrices = backupM;

		//for ( int i = 0; i < MATRIX_COUNT; ++i )
		//	std::memcpy( this->m_pMatrices[ i ], target.m_pMatrices[ i ], sizeof( matrix3x4_t[ 256 ] ) );

		return;
	}

	switch ( Config::Get<int>( Vars.RagebotTargetSelection ) )
	{
	case 0: {// highest damage
		if ( target.m_iBestDamage > this->m_iBestDamage )
			*this = target;
	}break;
	case 1: // fov
	{
		const auto firstFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, target.m_pPlayer->GetAbsOrigin( ) ) ) };
		const auto secondFov{ Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, this->m_pPlayer->GetAbsOrigin( ) ) ) };
		if ( firstFov < secondFov )
			*this = target;

		break;
	}
	case 2: // lowest distance
	{
		const float delta1{ target.m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };
		const float delta2{ this->m_pRecord->m_cAnimData.m_vecOrigin.DistTo( ctx.m_pLocal->m_vecOrigin( ) ) };

		if ( delta1 < delta2 )
			*this = target;

		break;
	}
	case 3: // lowest health
	{
		if ( target.m_pPlayer->m_iHealth( ) < this->m_pPlayer->m_iHealth( ) )
			*this = target;

		break;
	}
	}
}

void AimTarget_t::ScanTarget( std::vector<EHitboxIndex>& hitboxes ) {
	const auto backup{ new LagBackup_t( this->m_pPlayer ) };

	auto& entry{ Features::AnimSys.m_arrEntries.at( this->m_pPlayer->Index( ) - 1 ) };

	std::memcpy( this->m_pPlayer->m_CachedBoneData( ).Base( ), RECORD_MATRIX( this->m_pRecord ), this->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto LBY{ this->m_pRecord->m_bLBYUpdate };

	const auto forceBaim{ ( Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).enabled
		|| ( entry.m_iMissedShots > THIS.MenuVars.RagebotForceBaimAfterX && THIS.MenuVars.RagebotForceBaimAfterXINT ) )
		/* || !LBY*/ };

	GetBestPoint( forceBaim, hitboxes );

	backup->Apply( this->m_pPlayer );
	delete backup;
}

int ClipRayToHitbox( const Ray_t& ray, mstudiobbox_t* box, matrix3x4_t& mat, CGameTrace& tr ) {
	int retval = -1;

	__asm {
		mov ecx, ray
		mov edx, box
		push tr
		push mat
		call Displacement::Sigs.ClipRayToHitbox
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

int AimTarget_t::SafePoint( Vector aimpoint, int index ) {
	const auto hitboxSet{ this->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( this->m_pPlayer->m_nHitboxSet( ) ) };
	const auto hitbox{ hitboxSet->GetHitbox( index ) };
	if ( !hitbox )
		return 0;

	const auto dir{ ( aimpoint - ctx.m_vecEyePos ).Normalized( ) };
	const auto end{ ctx.m_vecEyePos + dir * ctx.m_pWeaponData->flRange };

	int hits{ };
	for ( int i{ }; i < DESYNC_MATRIX_COUNT; i++ ) {
		if ( intersectHitbox( end, hitbox, this->m_pRecord->m_cAnimData.m_arrSides.at( i ).m_pMatrix ) )
			hits++;
	}

	std::memcpy( this->m_pPlayer->m_CachedBoneData( ).Base( ), RECORD_MATRIX( this->m_pRecord ), this->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	return hits;
}

int OffsetDelta( CBasePlayer* player, LagRecord_t* record ) {
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

void AimTarget_t::GetBestPoint( bool forceBaim, std::vector<EHitboxIndex>& hitboxes ) {
	const auto hitboxSet{ this->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( this->m_pPlayer->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return;

	int mindmg{ std::max( THIS.MenuVars.RagebotPenetrationDamage, THIS.MenuVars.RagebotPenetrationDamage ) };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = THIS.MenuVars.RagebotOverrideDamage;
	else if ( THIS.MenuVars.RagebotScaleDamage )
		mindmg *= this->m_pPlayer->m_iHealth( ) / 100.f;

	for ( const auto& hb : hitboxes ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		// ignore limbs when moving
		if ( THIS.MenuVars.RagebotIgnoreLimbs && ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 5.f
			&& ( hitbox->iGroup == HITGROUP_LEFTLEG || hitbox->iGroup == HITGROUP_RIGHTLEG
				|| hitbox->iGroup == HITGROUP_LEFTARM || hitbox->iGroup == HITGROUP_RIGHTARM ) )
			continue;

		if ( hb == HITBOX_HEAD && this->m_cPoint.m_bValid ) {
			// prefer baim
			if ( THIS.MenuVars.RagebotPreferBaim && this->m_cPoint.m_iHitgroup != HITBOX_HEAD )
				break;

			if ( THIS.MenuVars.RagebotPreferBaimLethal && this->m_cPoint.m_flDamage > this->m_pPlayer->m_iHealth( ) && this->m_cPoint.m_iHitgroup != HITBOX_HEAD )
				break;

			// prefer baim if dt
			const bool isDTEnabled{ ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled ) };
			if ( THIS.MenuVars.RagebotPreferBaimDoubletap && ( isDTEnabled && ctx.m_iTicksAllowed ) )
				break;
		}

		auto damage{ static_cast< float >( ctx.m_pWeaponData->iDamage ) };
		Features::Autowall.ScaleDamage( this->m_pPlayer, damage, ctx.m_pWeaponData->flArmorRatio, hitbox->iGroup, 4.f );
		// impossible
		if ( damage < mindmg )
			continue;

		int scale{ ( hb ? THIS.MenuVars.RagebotBodyScale : THIS.MenuVars.RagebotHeadScale ) };

		if ( !THIS.MenuVars.RagebotStaticPointscale ) {
			// dynamic multipoints:
			// check the side delta + inac
			scale = 100;

			// this will take us down to a maximum 35 scale
			scale -= std::floor( std::min( Features::EnginePrediction.Inaccuracy + Features::EnginePrediction.Spread, 0.15f ) * 333.333f );

			// now compare side delta
			scale -= std::min( 90 - std::abs( 90 - OffsetDelta( this->m_pPlayer, this->m_pRecord.get( ) ) ), scale );
		}

		float scaleFloat{ scale / 100.f };

		auto& matrix{ RECORD_MATRIX( this->m_pRecord )[ hitbox->iBone ] };

		const auto center{ Math::VectorTransform( ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f, matrix ) };

		Multipoint( hb, center, hitbox, scaleFloat, hitboxes );
	}
}

void AimTarget_t::ScanPoint( AimPoint_t& point, std::vector<EHitboxIndex>& hitboxes ) {
	const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, this->m_pPlayer, ctx.m_pWeaponData,
		ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
		ctx.m_vecEyePos, point.m_vecPoint, THIS.MenuVars.RagebotAutowall ) };

	*this->m_pDbgLog = _( "ND" );

	if ( static_cast< int >( data.dmg ) < 1 )
		return;

	point.m_flDamage = static_cast< int >( data.dmg );
	point.m_iHitgroup = data.hitgroup;// TODO: MAKE ENTIRE RAGEBOT RUN OFF HITBOX

	auto mindmg{ data.penetrationCount < 4 ? THIS.MenuVars.RagebotPenetrationDamage : THIS.MenuVars.RagebotMinimumDamage };

	if ( Config::Get<bool>( Vars.RagebotDamageOverride )
		&& Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		mindmg = THIS.MenuVars.RagebotOverrideDamage;
	else if ( THIS.MenuVars.RagebotScaleDamage )
		mindmg *= this->m_pPlayer->m_iHealth( ) / 100.f;

	*this->m_pDbgLog = _( "MD[" ) + std::to_string( point.m_flDamage ) + "]";

	if ( point.m_flDamage < mindmg )
		return;

	*this->m_pDbgLog = _( "SA" );

	THIS.m_bShouldStop = true;

	if ( std::find( hitboxes.begin( ), hitboxes.end( ), data.hitbox ) == hitboxes.end( ) )
		return;

	point.m_iIntersections = SafePoint( point.m_vecPoint, data.hitbox );

	if ( Config::Get<keybind_t>( Vars.RagebotForceSafePointKey ).enabled && point.m_iIntersections < DESYNC_MATRIX_COUNT )
		return;

	point.m_bValid = true;

	if ( !this->m_cPoint.m_bValid )
		this->m_cPoint = point;
	else if ( point.m_iIntersections >= this->m_cPoint.m_iIntersections ) {
		if ( point.m_flDamage > this->m_cPoint.m_flDamage )
			this->m_cPoint = point;
		else if ( point.m_iIntersections > this->m_cPoint.m_iIntersections )
			this->m_cPoint = point;
	}
}

void AimTarget_t::Multipoint( EHitboxIndex index, Vector center, mstudiobbox_t* hitbox, float scale, std::vector<EHitboxIndex>& hitboxes ) {
	auto& matrix{ RECORD_MATRIX( this->m_pRecord )[ hitbox->iBone ] };

	*this->m_pDbgLog = _( "MP" );

	if ( hitbox->flRadius == -1.f ) {
		AimPoint_t point{ center, hitbox->iGroup };
		ScanPoint( point, hitboxes );

		if ( THIS.IsMultiPointEnabled( index ) ) {
			const auto d{ ( hitbox->vecBBMin.x - center.x ) * scale };
			point = { Math::VectorTransform( { center.x + d, center.y, center.z }, matrix ), hitbox->iGroup };
			ScanPoint( point, hitboxes );

			const auto d0{ ( hitbox->vecBBMax.x - center.x ) * scale };
			point = { Math::VectorTransform( { center.x + d0, center.y, center.z }, matrix ), hitbox->iGroup };
			ScanPoint( point, hitboxes );
		}

		return;
	}

	AimPoint_t point{ };

	if ( THIS.IsMultiPointEnabled( index ) ) {
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

			// left
			RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( middle * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );

			// right
			ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( middle0 * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );

			// bottom
			ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );

			// top
			ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );
		}
		else {
			// left
			RayTracer::Ray ray = RayTracer::Ray( ctx.m_vecEyePos, center - ( up * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );

			// right
			ray = RayTracer::Ray( ctx.m_vecEyePos, center + ( up * 1000.0f ) );
			RayTracer::TraceFromCenter( ray, box, trace, RayTracer::Flags_RETURNEND );

			point = { ( ( trace.m_traceEnd - center ) * scale ) + center, hitbox->iGroup };
			ScanPoint( point, hitboxes );
		}
	}

	point = { center, hitbox->iGroup };
	ScanPoint( point, hitboxes );
}

LagRecord_t* AimTarget_t::GetAbuseRecord( PlayerEntry& entry ) {
	const auto curtime{ ctx.m_flFixedCurtime + TICKS_TO_TIME( 1 ) };
	const auto correct{ std::clamp( ctx.m_flOutLatency + ctx.m_flLerpTime, 0.f, Displacement::Cvars.sv_maxunlag->GetFloat( ) ) };
	const auto flTargetTime{ curtime - correct };
	const auto serverCurtime{ TICKS_TO_TIME( Interfaces::Globals->iTickCount + ctx.m_iRealOutLatencyTicks + 1 ) };
	const auto flDeadtime{ static_cast< int >( serverCurtime - Displacement::Cvars.sv_maxunlag->GetFloat( ) ) };

	std::shared_ptr< LagRecord_t > prevRecord{ };
	std::shared_ptr< LagRecord_t > record{ };
	for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		//if ( record->m_cAnimData.m_flSimulationTime < flDeadtime )
		//	continue;

		prevRecord = record;
		record = *it;

		if ( record->m_cAnimData.m_flSimulationTime <= flTargetTime )
			break;
	}

	LagRecord_t* ret{ new LagRecord_t( *record ) };

	float frac{ 0.f };
	if ( prevRecord &&
		( record->m_cAnimData.m_flSimulationTime < flTargetTime ) &&
		( record->m_cAnimData.m_flSimulationTime < prevRecord->m_cAnimData.m_flSimulationTime ) ) {
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		if ( prevRecord->m_cAnimData.m_flSimulationTime <= record->m_cAnimData.m_flSimulationTime ) {
			delete ret;
			return nullptr;
		}

		if ( flTargetTime >= prevRecord->m_cAnimData.m_flSimulationTime ) {
			delete ret;
			return nullptr;
		}

		// calc fraction between both records
		frac = ( flTargetTime - record->m_cAnimData.m_flSimulationTime ) /
			( prevRecord->m_cAnimData.m_flSimulationTime - record->m_cAnimData.m_flSimulationTime );

		// should never extrapolate
		if ( frac > 1 || frac < 0 ) {
			delete ret;
			return nullptr;
		}

		ret->m_cAnimData.m_vecOrigin = Math::Lerp( frac, record->m_cAnimData.m_vecOrigin, prevRecord->m_cAnimData.m_vecOrigin );
		ret->m_cAnimData.m_vecMins = Math::Lerp( frac, record->m_cAnimData.m_vecMins, prevRecord->m_cAnimData.m_vecMins );
		ret->m_cAnimData.m_vecMaxs = Math::Lerp( frac, record->m_cAnimData.m_vecMaxs, prevRecord->m_cAnimData.m_vecMaxs );

		ret->Apply( entry.m_pPlayer );

		if ( record->m_cAnimData.m_arrSides.at( 1 ).m_cAnimState.flAbsYaw != record->m_cAnimData.m_arrSides.at( 2 ).m_cAnimState.flAbsYaw ) {
			for ( int i{ }; i < 3; ++i ) {
				const auto ang{ Math::Lerp( frac, record->m_cAnimData.m_arrSides.at( i ).m_cAnimState.flAbsYaw, prevRecord->m_cAnimData.m_arrSides.at( i ).m_cAnimState.flAbsYaw ) };

				auto& side{ ret->m_cAnimData.m_arrSides.at( i ) };

				entry.m_pPlayer->SetAbsAngles( { 0, ang, 0 } );
				entry.m_pPlayer->m_flPoseParameter( ) = side.m_flPoseParameter;
				*entry.m_pPlayer->m_pAnimState( ) = side.m_cAnimState;

				std::memcpy( entry.m_pPlayer->m_AnimationLayers( ), record->m_cAnimData.m_pLayers, 0x38 * 13 );

				Features::AnimSys.SetupBonesRebuilt( entry.m_pPlayer, side.m_pMatrix,
					BONE_USED_BY_SERVER, curtime, false );
			}
		}
		else {
			const auto ang{ Math::Lerp( frac, record->m_cAnimData.m_arrSides.at( 0 ).m_cAnimState.flAbsYaw, prevRecord->m_cAnimData.m_arrSides.at( 0 ).m_cAnimState.flAbsYaw ) };

			auto& side{ ret->m_cAnimData.m_arrSides.at( 0 ) };

			entry.m_pPlayer->SetAbsAngles( { 0, ang, 0 } );
			entry.m_pPlayer->m_flPoseParameter( ) = side.m_flPoseParameter;
			*entry.m_pPlayer->m_pAnimState( ) = side.m_cAnimState;

			std::memcpy( entry.m_pPlayer->m_AnimationLayers( ), record->m_cAnimData.m_pLayers, 0x38 * 13 );

			Features::AnimSys.SetupBonesRebuilt( entry.m_pPlayer, side.m_pMatrix,
				BONE_USED_BY_SERVER, curtime, false );

			for ( int i{ 1 }; i < 3; ++i )
				std::memcpy( ret->m_cAnimData.m_arrSides.at( i ).m_pMatrix, side.m_pMatrix, 256 * sizeof( matrix3x4a_t ) );
		}

		/*for ( int i{ }; i < 3; ++i ) {
			auto& mat{ ret->m_cAnimData.m_arrSides.at( i ).m_pMatrix };

			for ( std::size_t j{ }; j < 256; ++j ) {
				auto& boneNew{ mat[ j ] };

				boneNew.SetOrigin( boneNew.GetOrigin( ) + delta );
			}
		}*/
	}

	return ret;
}

void AimTarget_t::Extrapolate( PlayerEntry& entry, bool useLagRecord ) {
	int delay{ };
	if ( ctx.m_bFakeDucking )
		delay = 15 - Interfaces::ClientState->nChokedCommands;

	auto latency{ ctx.m_iRealOutLatencyTicks + ( Interfaces::Globals->iTickCount - entry.m_iLastRecievedTick ) + delay };
	if ( ctx.m_pLocal->Index( ) < this->m_pPlayer->Index( ) )
		latency -= 1;

	if ( entry.m_pRecords.back( )->m_bFirst )
		return;

	const auto curtime{ ctx.m_flFixedCurtime + TICKS_TO_TIME( 1 ) };

	const auto correct{ std::clamp( ctx.m_flOutLatency + ctx.m_flLerpTime, 0.f, Displacement::Cvars.sv_maxunlag->GetFloat( ) ) };

	float simtime{ this->m_pPlayer->m_flSimulationTime( ) };

	// cap it out to the newest record and ignore the current pos
	auto extrapolationCapped{ static_cast< int >( ( latency ) / entry.m_iLastNewCmds ) * entry.m_iLastNewCmds };

	if ( useLagRecord ) {
		const auto extrapCapped{ extrapolationCapped };
		for ( int i{ 0 }; i <= extrapCapped; i += entry.m_iLastNewCmds ) {
			extrapolationCapped = i;

			const auto delta{ correct - ( curtime - simtime + TICKS_TO_TIME( i ) ) };

			if ( simtime + TICKS_TO_TIME( i ) > entry.m_flHighestSimulationTime
				&& std::fabsf( delta ) < 0.2f )
				break;
		}

		if ( extrapolationCapped > extrapCapped ) {
			*this->m_pDbgLog = _( "N1" );
			return;
		}


		simtime = this->m_pPlayer->m_flSimulationTime( ) + TICKS_TO_TIME( extrapolationCapped );

		if ( simtime < entry.m_flHighestSimulationTime ) {
			*this->m_pDbgLog = _( "N2" );
			return;
		}

		//extrapolationCapped = static_cast< int >( ( extrapolationCapped ) / entry.m_iLastRecordInterval ) * entry.m_iLastRecordInterval;
	}

	ExtrapolationBackup_t backupExtrapolationData{ entry.m_pPlayer };

	if ( extrapolationCapped ) {
		if ( !THIS.ExtrapolatePlayer( entry, -1, 0, extrapolationCapped, entry.m_vecPreviousVelocity ) ) {
			this->m_pRecord = nullptr;
			*this->m_pDbgLog = _( "EP" );
			return;
		}

		const auto newRecord{ std::make_shared< LagRecord_t >( entry.m_pPlayer ) };
		newRecord->m_iReceiveTick = Interfaces::Globals->iTickCount + extrapolationCapped;

		auto& side{ newRecord->m_cAnimData.m_arrSides.at( 0 ) };

		newRecord->m_cAnimData.m_flSimulationTime = simtime;

		Features::AnimSys.SetupBonesRebuilt( entry.m_pPlayer, side.m_pMatrix,
			BONE_USED_BY_SERVER, newRecord->m_cAnimData.m_flSimulationTime + TICKS_TO_TIME( extrapolationCapped ), false );

		// TODO: should i bother setting up all sides
		for ( int i{ 1 }; i < 3; ++i )
			std::memcpy( newRecord->m_cAnimData.m_arrSides.at( i ).m_pMatrix, side.m_pMatrix, 256 * sizeof( matrix3x4a_t ) );

		this->m_pRecord = newRecord;

		backupExtrapolationData.restore( entry.m_pPlayer );
	}
	else
		this->m_pRecord = entry.m_pRecords.back( );

	if ( !this->m_pRecord->Validity( ) && Config::Get<bool>( Vars.RagebotLagcompensation ) ) {
		this->m_pRecord = nullptr;
		*this->m_pDbgLog = _( "VD" );
		return;
	}

	this->m_bExtrapolating = true;

	std::vector<int> hitgroups{ HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT };
	const auto dmg{ QuickScan( this->m_pRecord, hitgroups ) };

	if ( dmg < 1 )
		this->m_pRecord = nullptr;
	else
		this->m_iBestDamage = dmg;
}

void AimTarget_t::GetBestLagRecord( PlayerEntry& entry ) {
	const auto backup{ new LagBackup_t( this->m_pPlayer ) };

	if ( !Config::Get<bool>( Vars.RagebotLagcompensation ) ) {
	EXTRAPOLATE:
		Extrapolate( entry, Config::Get<bool>( Vars.RagebotLagcompensation ) );
		backup->Apply( entry.m_pPlayer );
		delete backup;
		return;
	}

	//if ( const auto& record{ entry.m_pRecords.back( ) }; record->m_bBrokeLC /* || ( entry.m_pPlayer->m_vecOrigin( ) - entry.m_pRecords.back( )->m_cAnimData.m_vecOrigin ).LengthSqr( ) > 4096.f*/ )
	//	goto EXTRAPOLATE;

	std::shared_ptr< LagRecord_t > oldestRecord{ };
	LagRecord_t* previous{ };
	for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
		const auto& record{ *it };

		if ( record->BreakingLagcompensation( previous ) )
			break;

		previous = record.get( );

		if ( record->m_bFirst )
			continue;

		const auto validity{ record->Validity( ) };
		if ( !validity )// **( int** ) Displacement::Sigs.numticks * 2 )
			continue;

		if ( validity < 5
			&& ctx.m_iTicksAllowed )
			continue;

		if ( Config::Get<keybind_t>( Vars.RagebotForceYawSafetyKey ).enabled
			&& !record->m_bLBYUpdate )
			continue;

		oldestRecord = record;
	}

	std::vector <int> hitgroups{ HITBOX_HEAD, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT };//HITBOX_STOMACH
	const auto& idx{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) };
	const auto hitboxSet{ this->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( this->m_pPlayer->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return;
	const auto head{ hitboxSet->GetHitbox( HITBOX_HEAD ) };

	if ( oldestRecord ) {
		const auto dmg{ this->QuickScan( oldestRecord, hitgroups ) };
		if ( dmg > 0 ) {
			this->m_iBestDamage = dmg;
			this->m_pRecord = oldestRecord;
		}

		//bool first{ };

		for ( auto it{ entry.m_pRecords.rbegin( ) }; it != entry.m_pRecords.rend( ); it = std::next( it ) ) {
			const auto& record{ *it };

			if ( record == oldestRecord )
				break;

			if ( record->BreakingLagcompensation( previous ) )
				break;

			previous = record.get( );

			if ( record->m_bFirst )
				continue;

			const auto validity{ record->Validity( ) };
			if ( !validity )
				continue;

			// highest tickbase error we can get is +/- 2
			if ( validity < 5
				&& ctx.m_iTicksAllowed )
				continue;

			const auto LBY{ record->m_bLBYUpdate };
			const auto oldRecordLBY{ this->m_pRecord && this->m_pRecord->m_bLBYUpdate };

			if ( Config::Get<keybind_t>( Vars.RagebotForceYawSafetyKey ).enabled
				&& !LBY )
				continue;

			const auto dmg{ this->QuickScan( record, hitgroups ) };
			if ( hitgroups.size( ) > 1 ) {
				hitgroups = { ( idx == WEAPON_SCAR20 || idx == WEAPON_G3SG1 || idx == WEAPON_AWP )
					? HITBOX_STOMACH : HITBOX_HEAD };
			}

			if ( dmg <= 0 ) {
				if ( !this->m_pRecord )
					break;

				continue;
			}

			/*if ( !first ) {
				int delay{ };
				if ( ctx.m_bFakeDucking )
					delay = 15 - Interfaces::ClientState->nChokedCommands;

				auto latency{ ctx.m_iRealOutLatencyTicks + ( Interfaces::Globals->iTickCount - entry.m_iLastRecievedTick ) + delay };
				if ( ctx.m_pLocal->Index( ) < this->m_pPlayer->Index( ) )
					latency -= 1;

				this->m_flYaw = THIS.ExtrapolateYaw( entry.m_flPreviousYaws, latency );
			}

			first = true;*/

			if ( dmg >= this->m_iBestDamage
				|| this->m_iBestDamage - dmg <= 10 ) {
				const auto point{ Math::VectorTransform( ( head->vecBBMin + head->vecBBMax ) / 2.f, RECORD_MATRIX( record )[ head->iBone ] ) };

				if ( dmg >= this->m_iBestDamage + 10 ) {
					this->m_iBestDamage = dmg;
					this->m_pRecord = record;
				}
				else if ( LBY ) {
					if ( !oldRecordLBY ) {
						this->m_pRecord = record;
					}
					else {
						if ( dmg > this->m_iBestDamage ) {
							this->m_pRecord = record;
							this->m_iBestDamage = dmg;
						}
					}

					/*else if ( safepoints == bestSafepoints
						&& this->m_pRecord
						&& std::abs( Math::AngleDiff( this->m_flYaw, record->m_angEyeAngles.y ) )
						< std::abs( Math::AngleDiff( this->m_flYaw, this->m_pRecord->m_angEyeAngles.y ) ) )
						this->m_pRecord = record;*/
				}
				else if ( LBY == oldRecordLBY 
					&& this->m_pRecord == oldestRecord )
					this->m_pRecord = record;
			}
		}
	}
	else {
		this->m_pDbgLog = ctx.m_strDbgLogs.emplace_back( std::make_shared< std::string >( _( "NO" ) ) );
		if ( Config::Get<bool>( Vars.DBGExtrap ) )
			goto EXTRAPOLATE;
	}

	backup->Apply( entry.m_pPlayer );

	delete backup;
}

int AimTarget_t::QuickScan( std::shared_ptr< LagRecord_t > record, std::vector<int>& hitgroups ) {
	//this->SetupMatrices( record );

	std::memcpy( this->m_pPlayer->m_CachedBoneData( ).Base( ), RECORD_MATRIX( record ), this->m_pPlayer->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto hitboxSet{ this->m_pPlayer->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( this->m_pPlayer->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return 0;

	int dmg{ };

	for ( const auto& hb : hitgroups ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			continue;

		const auto point{ Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, RECORD_MATRIX( record )[ hitbox->iBone ] ) };

		const auto data{ Features::Autowall.FireBullet( ctx.m_pLocal, this->m_pPlayer, ctx.m_pWeaponData,
			ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER,
			ctx.m_vecEyePos, point, THIS.MenuVars.RagebotAutowall ) };

		if ( static_cast< int >( data.dmg ) < 1 )
			continue;

		if ( data.dmg > dmg )
			dmg = data.dmg;

		if ( data.hitbox == HITBOX_HEAD )
			break;
	}

	return dmg;
}