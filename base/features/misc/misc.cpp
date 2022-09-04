#include "misc.h"
#include "../rage/ragebot.h"

void CMisc::Movement( CUserCmd& cmd ) {
	if ( ctx.m_pLocal->IsDead( ) )
		return;

	if ( ctx.m_pLocal->m_MoveType( ) != MOVETYPE_WALK )
		return;

	if ( ctx.m_pWeapon && ctx.m_pWeaponData
		&& !ctx.m_bCanShoot
		&& ( ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4
			&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ) )
		cmd.iButtons &= ~IN_ATTACK;

	MovementAngle = cmd.viewAngles;

	m_bWasJumping = cmd.iButtons & IN_JUMP;

	m_ve2OldMovement = { cmd.flForwardMove, cmd.flSideMove };

	if ( Config::Get<bool>(Vars.MiscInfiniteStamina ) )
		cmd.iButtons |= IN_BULLRUSH;

	FakeDuck( cmd );

	if ( !AutoStop( cmd ) ) {
		if ( !MicroMove( cmd ) ) {
			QuickStop( cmd );

			if ( Config::Get<bool>( Vars.MiscBunnyhop ) && m_bWasJumping ) {
				if ( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND )
					cmd.iButtons |= IN_JUMP;
				else
					cmd.iButtons &= ~IN_JUMP;
			}

			SlowWalk( cmd );
			AutoStrafer( cmd );
		}
	}

	cmd.flForwardMove = std::clamp<float>( cmd.flForwardMove, -450.f, 450.f );
	cmd.flSideMove = std::clamp<float>( cmd.flSideMove, -450.f, 450.f );
	cmd.flUpMove = std::clamp<float>( cmd.flUpMove, -320.f, 320.f );
}

void CMisc::QuickStop( CUserCmd& cmd ) {
	if ( Config::Get<bool>( Vars.MiscQuickStop ) 
		&& !cmd.flForwardMove && !cmd.flSideMove 
		&& !( cmd.iButtons & IN_JUMP ) 
		&& ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND ) {
		cmd.iButtons &= ~( IN_SPEED | IN_WALK );

		if ( ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 20.f ) {
			QAngle direction;
			Math::VectorAngles( ctx.m_pLocal->m_vecVelocity( ), direction );
			direction.y = ctx.m_angOriginalViewangles.y - direction.y;

			Vector forward;
			Math::AngleVectors( direction, &forward );

			const Vector negated_direction = forward * -ctx.m_pLocal->m_vecVelocity( ).Length2D( );

			cmd.flForwardMove = negated_direction.x;
			cmd.flSideMove = negated_direction.y;
		}
		else
			cmd.flForwardMove = cmd.flSideMove = 0;
	}
}

void CMisc::NormalizeMovement( CUserCmd& cmd ) {
	cmd.viewAngles.Normalize( );
	cmd.viewAngles.Clamp( );

	//cmd.flForwardMove = std::clamp<float>( cmd.flForwardMove, -450.f, 450.f );
	//cmd.flSideMove = std::clamp<float>( cmd.flSideMove, -450.f, 450.f );
	//cmd.flUpMove = std::clamp<float>( cmd.flUpMove, -320.f, 320.f );

	if ( ctx.m_pLocal->m_MoveType( ) != MOVETYPE_WALK )
		return;

	cmd.iButtons &= ~( IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT );

	if ( cmd.flForwardMove != 0.f )
		cmd.iButtons |=
		( Config::Get<bool>( Vars.MiscSlideWalk ) ? cmd.flForwardMove < 0.f : cmd.flForwardMove > 0.f )
		? IN_FORWARD : IN_BACK;

	if ( cmd.flSideMove == 0.f )
		return;

	cmd.iButtons |=
		( Config::Get<bool>( Vars.MiscSlideWalk ) ? cmd.flSideMove < 0.f : cmd.flSideMove > 0.f )
		? IN_MOVERIGHT : IN_MOVELEFT;
}

void CMisc::MoveMINTFix( CUserCmd& cmd, QAngle wish_angles, int flags, int move_type ) {
	if ( cmd.viewAngles.x == wish_angles.x && cmd.viewAngles.y == wish_angles.y && cmd.viewAngles.z == wish_angles.z )
		return;

	if ( cmd.viewAngles.z != 0.f
		&& !( flags & FL_ONGROUND ) )
		cmd.flSideMove = 0.f;

	auto move_2d = Vector2D( cmd.flForwardMove, cmd.flSideMove );

	if ( const auto speed_2d = move_2d.Length( ) ) {
		const auto delta = cmd.viewAngles.y - wish_angles.y;

		Vector2D v1;

		Math::SinCos(
			DEG2RAD(
				std::remainder(
					RAD2DEG(
						std::atan2( move_2d.y / speed_2d, move_2d.x / speed_2d )
					) + delta, 360.f
				)
			), v1.x, v1.y
		);

		const auto cos_x = std::cos(
			DEG2RAD(
				std::remainder(
					RAD2DEG( std::atan2( 0.f, speed_2d ) ), 360.f
				)
			)
		);

		move_2d.x = cos_x * v1.y * speed_2d;
		move_2d.y = cos_x * v1.x * speed_2d;

		if ( move_type == MOVETYPE_LADDER ) {
			if ( wish_angles.x < 45.f
				&& std::abs( delta ) <= 65.f
				&& cmd.viewAngles.x >= 45.f ) {
				move_2d.x *= -1.f;
			}
		}
		else if ( std::abs( cmd.viewAngles.x ) > 90.f )
			move_2d.x *= -1.f;
	}

	cmd.flForwardMove = move_2d.x;
	cmd.flSideMove = move_2d.y;

	cmd.iButtons &= ~( IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT );

	if ( move_type == MOVETYPE_LADDER ) {
		if ( std::abs( cmd.flForwardMove ) > 200.f )
			cmd.iButtons |=
			cmd.flForwardMove > 0.f
			? IN_FORWARD : IN_BACK;

		if ( std::abs( cmd.flSideMove ) <= 200.f )
			return;

		cmd.iButtons |=
			cmd.flSideMove > 0.f
			? IN_MOVERIGHT : IN_MOVELEFT;

		return;
	}

	if ( Config::Get<bool>( Vars.MiscSlideWalk )
		&& move_type == MOVETYPE_WALK ) {
		if ( cmd.flForwardMove != 0.f )
			cmd.iButtons |=
			cmd.flForwardMove < 0.f
			? IN_FORWARD : IN_BACK;

		if ( cmd.flSideMove == 0.f )
			return;

		cmd.iButtons |=
			cmd.flSideMove < 0.f
			? IN_MOVERIGHT : IN_MOVELEFT;

		return;
	}

	if ( cmd.flForwardMove != 0.f )
		cmd.iButtons |=
		cmd.flForwardMove > 0.f
		? IN_FORWARD : IN_BACK;

	if ( cmd.flSideMove == 0.f )
		return;

	cmd.iButtons |=
		cmd.flSideMove > 0.f
		? IN_MOVERIGHT : IN_MOVELEFT;
}

bool CMisc::MicroMove( CUserCmd& cmd ) {
	if ( !Config::Get<bool>( Vars.AntiaimEnable )
		|| !Config::Get<bool>( Vars.AntiaimDesync )
		|| cmd.iButtons & IN_JUMP
		|| !( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND ) )
		return false;

	if ( ctx.m_pLocal->m_vecVelocity( ).Length2DSqr( ) > 2.f )
		return false;

	cmd.iButtons &= ~IN_SPEED;

	float duck_amount{ };
	if ( cmd.iButtons & IN_DUCK )
		duck_amount = std::min(
			1.f,
			ctx.m_pLocal->m_flDuckAmount( )
			+ ( Interfaces::Globals->flIntervalPerTick * 0.8f ) * ctx.m_pLocal->m_flDuckSpeed( )
		);
	else
		duck_amount =
		ctx.m_pLocal->m_flDuckAmount( )
		- std::max( 1.5f, ctx.m_pLocal->m_flDuckSpeed( ) ) * Interfaces::Globals->flIntervalPerTick;

	float move{ };
	if ( cmd.iButtons & IN_DUCK
		|| ctx.m_pLocal->m_flDuckAmount( )
		|| ctx.m_pLocal->m_fFlags( ) & FL_DUCKING )
		move = 1.1f / ( ( ( duck_amount * 0.34f ) + 1.f ) - duck_amount );
	else
		move = 1.1f;

	if ( std::abs( cmd.flForwardMove ) > move
		|| std::abs( cmd.flSideMove ) > move )
		return false;

	static bool sw = false;
	sw = !sw;

	if ( !sw )
		move *= -1.f;

	cmd.flSideMove = move;

	return true;
}

void CMisc::AutoStrafer( CUserCmd& cmd ) {
	if ( !Config::Get<bool>( Vars.MiscAutostrafe ) )

		return;
	if ( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND )
		return;

	const auto& velocity = ctx.m_pLocal->m_vecVelocity( );

	if ( velocity.Length2D( ) < 2.f
		&& !cmd.flForwardMove && !cmd.flSideMove )
		return;

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled )
		return;

	if ( cmd.flForwardMove != 0.f || cmd.flSideMove != 0.f )
		MovementAngle.y = std::remainder(
			MovementAngle.y
			+ std::remainder(
				RAD2DEG(
					std::atan2( cmd.flForwardMove, cmd.flSideMove )
				) - 90.f, 360.f
			), 360.f
		);

	cmd.flForwardMove = cmd.flSideMove = 0.f;

	const auto speed_2d = velocity.Length2D( );

	const auto ideal_strafe = std::min( 90.f, RAD2DEG( std::asin( 15.f / speed_2d ) ) );

	static bool m_strafe_switch = false;

	const auto mult = m_strafe_switch ? 1.f : -1.f;

	m_strafe_switch = !m_strafe_switch;

	auto delta = std::remainder( MovementAngle.y - OldYaw, 360.f );
	if ( delta )
		cmd.flSideMove = delta < 0.f ? 450.f : -450.f;

	OldYaw = MovementAngle.y;

	delta = std::abs( delta );

	if ( delta >= 30.f
		|| ideal_strafe >= delta ) {
		const auto vel_angle = RAD2DEG( std::atan2( velocity.y, velocity.x ) );
		const auto vel_delta = std::remainder( MovementAngle.y - vel_angle, 360.f );

		if ( speed_2d <= 15.f
			|| ideal_strafe >= vel_delta ) {
			if ( speed_2d <= 15.f
				|| vel_delta >= -ideal_strafe ) {
				cmd.flSideMove = 450.f * mult;
				MovementAngle.y += ideal_strafe * mult;
			}
			else {
				cmd.flSideMove = 450.f;
				MovementAngle.y = vel_angle - ideal_strafe;
			}
		}
		else {
			cmd.flSideMove = -450.f;
			MovementAngle.y = vel_angle + ideal_strafe;
		}
	}

	MoveMINTFix( cmd, MovementAngle, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
}

void CMisc::SlowWalk( CUserCmd& cmd ) {
	if ( !ctx.m_pWeaponData )
		return;

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled ) {
		cmd.iButtons &= ~( IN_SPEED | IN_WALK );

		const float opt_speed = ( ctx.m_pLocal->m_bIsScoped( ) ? ctx.m_pWeaponData->flMaxSpeedAlt : ctx.m_pWeaponData->flMaxSpeed ) / 3.f;
		const float movement_speed = std::sqrtf( cmd.flSideMove * cmd.flSideMove ) + ( cmd.flForwardMove * cmd.flForwardMove ) + ( cmd.flUpMove * cmd.flUpMove );
		float speed = ctx.m_pLocal->m_vecVelocity( ).Length2D( );

		LimitSpeed( cmd, opt_speed );
	}
}

void CMisc::FakeDuck( CUserCmd& cmd ) {
	const auto Prev = ctx.m_bFakeDucking;

	ctx.m_bFakeDucking = Config::Get<bool>( Vars.MiscFakeDuck ) && Config::Get<keybind_t>( Vars.MiscFakeDuckKey ).enabled && ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND && int( 1.0f / Interfaces::Globals->flIntervalPerTick ) == 64;

	if ( !Prev ) {
		if ( !ctx.m_bFakeDucking )
			return;

		if ( ctx.m_bSendPacket ) {
			cmd.iButtons |= IN_BULLRUSH;
			cmd.iButtons &= ~IN_DUCK;
		}
		else
			ctx.m_bSendPacket = true;

		return;
	}
	if ( !( cmd.iButtons & IN_BULLRUSH ) )
		cmd.iButtons | IN_BULLRUSH;

	if ( Interfaces::ClientState->nChokedCommands <= 6u )
		cmd.iButtons &= ~IN_DUCK;
	else
		cmd.iButtons |= IN_DUCK;

	ctx.m_bSendPacket = Interfaces::ClientState->nChokedCommands >= 14u;
}

void CMisc::AutoPeek( CUserCmd& cmd ) {
	if ( !Config::Get<bool>( Vars.MiscAutoPeek ) 
		|| !ctx.m_pWeapon
		|| ctx.m_pWeapon->IsKnife( ) || ctx.m_pWeapon->IsGrenade( )
		|| ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER )
		return;

	const auto origin = ctx.m_pLocal->GetAbsOrigin( );

	if ( Config::Get<keybind_t>( Vars.MiscAutoPeekKey ).enabled ) {
		if ( !AutoPeeking ) {
			ShouldRetract = false;
			OldOrigin = ctx.m_pLocal->GetAbsOrigin( );

			Ray_t ray{ OldOrigin, OldOrigin - Vector( 0.0f, 0.0f, 1000.0f ) };
			CTraceFilter filter{ ctx.m_pLocal, TRACE_WORLD_ONLY };
			CGameTrace trace;

			Interfaces::EngineTrace->TraceRay( ray, MASK_SOLID, &filter, &trace );

			if ( trace.flFraction < 1.0f )
				OldOrigin = trace.vecEnd + Vector( 0.0f, 0.0f, 2.0f );
		}
		if ( ShouldRetract ) {
			const auto angle = Math::CalcAngle( ctx.m_pLocal->GetAbsOrigin( ), OldOrigin );
			MovementAngle.y = angle.y;

			cmd.flForwardMove = 450.f;
			cmd.flSideMove = 0.f;

			MoveMINTFix( cmd, MovementAngle, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );

			const auto distance = origin.DistTo( OldOrigin );
			if ( distance <= 10.0 )
				ShouldRetract = false;
		}
		if ( cmd.iButtons & IN_ATTACK )
			ShouldRetract = true;

		AutoPeeking = true;
	}
	else {
		AutoPeeking = false;
		ShouldRetract = false;
	}
}

void CMisc::Thirdperson( ) {
	if ( !Interfaces::Input->bCameraInThirdPerson )
		Interfaces::Input->bCameraInThirdPerson = true;

	Vector camForward;
	QAngle camAngles;

	QAngle angles;
	Interfaces::Engine->GetViewAngles( angles );

	camAngles.x = angles.x;
	camAngles.y = angles.y;
	camAngles.z = 0;

	Math::AngleVectors( camAngles, &camForward, 0, 0 );

	camAngles.z = Config::Get<int>( Vars.VisThirdPersonDistance );

	const auto eyeorigin = ctx.m_pLocal->GetAbsOrigin( ) + ( ctx.m_bFakeDucking ? Vector( 0, 0, Interfaces::GameMovement->GetPlayerViewOffset( false ).z ) : ctx.m_pLocal->m_vecViewOffset( ) );

	Vector vecCamOffset( eyeorigin - ( camForward * camAngles.z ) );

	Ray_t ray{ eyeorigin, vecCamOffset, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) };
	CGameTrace tr;
	CTraceFilter filter{ ctx.m_pLocal, TRACE_WORLD_ONLY };

	Interfaces::EngineTrace->TraceRay( ray, MASK_NPCWORLDSTATIC, &filter, &tr );

	TPFrac = Math::Interpolate( TPFrac, tr.flFraction, Interfaces::Globals->flFrameTime * 10.f );

	camAngles.z *= TPFrac;

	Interfaces::Input->vecCameraOffset.x = camAngles.x;
	Interfaces::Input->vecCameraOffset.y = camAngles.y;
	Interfaces::Input->vecCameraOffset.z = camAngles.z;
}

bool CMisc::InPeek( ) {
	if ( ctx.m_pLocal->m_vecVelocity( ).Length( ) < 0.2f ) {
		Features::Exploits.m_bAlreadyPeeked = false;
		return false;
	}

	matrix3x4_t backup_matrix[ 256 ];
	memcpy( backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto delta = ctx.m_pLocal->m_vecVelocity( ) * ( TICKS_TO_TIME( 3 /* + ctx.m_flOutLatency */ ) );

	// credits: diamondhack!
	for ( std::size_t i{ }; i < ctx.m_pLocal->m_CachedBoneData( ).Count( ); ++i ) {
		auto& bone = ctx.m_pLocal->m_CachedBoneData( ).Base( )[ i ];

		bone[ 0 ][ 3 ] += delta.x;
		bone[ 1 ][ 3 ] += delta.y;
		bone[ 2 ][ 3 ] += delta.z;
	}

	const auto backup_origin = ctx.m_pLocal->m_vecOrigin( );
	const auto backupAbsOrigin = ctx.m_pLocal->GetAbsOrigin( );

	ctx.m_pLocal->m_vecOrigin( ) += delta;
	ctx.m_pLocal->SetAbsOrigin( ctx.m_pLocal->m_vecOrigin( ) );

	const auto hitboxSet = ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) );
	if ( !hitboxSet )
		return false;

	auto bestFOV{ INT_MAX };
	CBasePlayer* bestPlayer{ nullptr };
	for ( auto i = 1; i <= 64; ++i ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player
			|| !player->IsPlayer( )
			|| player->Dormant( )
			|| player->IsDead( )
			|| player->IsTeammate( ) )
			continue;

		const auto fov = Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) );
		if ( fov > bestFOV )
			continue;

		bestPlayer = player;
	}

	if ( !bestPlayer ) {
		ctx.m_pLocal->m_vecOrigin( ) = backup_origin;
		ctx.m_pLocal->SetAbsOrigin( backupAbsOrigin );
		memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
		return false;
	}

	auto enemyShootPos = bestPlayer->m_vecOrigin( );
	enemyShootPos.z += 64.f;

	int bestDmg{ };

	for ( const auto& hb : { HITBOX_CHEST, HITBOX_RIGHT_THIGH, HITBOX_LEFT_THIGH, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {
		const auto hitbox = hitboxSet->GetHitbox( hb );
		if ( !hitbox )
			return false;

		Vector center = ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f;
		center = Math::VectorTransform( center, ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] );

		const auto data{ Features::Autowall.FireEmulated( bestPlayer, ctx.m_pLocal,
			enemyShootPos, center ) };

		if ( data.dmg > bestDmg ) {
			bestDmg = data.dmg;
			break;
		}
	}

	if ( bestDmg > 1 ) {
		ctx.m_pLocal->m_vecOrigin( ) = backup_origin;
		ctx.m_pLocal->SetAbsOrigin( backup_origin );
		memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
		return true;
	}

	ctx.m_pLocal->m_vecOrigin( ) = backup_origin;
	ctx.m_pLocal->SetAbsOrigin( backupAbsOrigin );
	memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), backup_matrix, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
	Features::Exploits.m_bAlreadyPeeked = false;
	return false;
}