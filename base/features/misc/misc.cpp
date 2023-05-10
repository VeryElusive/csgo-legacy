#include "misc.h"
#include "../rage/ragebot.h"
#include "../../utils/threading/thread_pool.hpp"

void CMisc::Movement( CUserCmd& cmd ) {
	if ( ctx.m_pLocal->m_MoveType( ) != MOVETYPE_WALK )
		return;

	if ( ctx.m_pWeapon && ctx.m_pWeaponData
		&& !ctx.m_bCanShoot
		&& ( ctx.m_pWeaponData->nWeaponType < WEAPONTYPE_C4
			&& ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ) )
		cmd.iButtons &= ~IN_ATTACK;

	MovementAngle = cmd.viewAngles;

	m_bWasJumping = cmd.iButtons & IN_JUMP;

	if ( Config::Get<bool>( Vars.MiscInfiniteStamina ) )
		cmd.iButtons |= IN_BULLRUSH;

	m_ve2OldMovement = { cmd.flForwardMove, cmd.flSideMove };

	FakeDuck( cmd );

	if ( !AutoStop( cmd ) ) {
		/*if ( !MicroMove( cmd ) )*/ {
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

bool CMisc::AutoStop( CUserCmd& cmd ) {
	if ( !Features::Ragebot.m_bShouldStop )
		return false;

	ctx.m_iLastStopTime = Interfaces::Globals->flRealTime;

	if ( !Features::Ragebot.RagebotAutoStop )
		return false;

	if ( !Config::Get<bool>( Vars.RagebotEnable ) )
		return false;

	if ( cmd.iButtons & IN_JUMP
		|| !( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND ) )
		return false;

	if ( Features::Ragebot.RagebotBetweenShots && !ctx.m_bCanShoot )
		return false;

	if ( !ctx.m_pWeaponData )
		return false;

	const auto maxWeaponSpeed{ ( ctx.m_pLocal->m_bIsScoped( ) ? ctx.m_pWeaponData->flMaxSpeedAlt : ctx.m_pWeaponData->flMaxSpeed ) };
	const auto optSpeed{ maxWeaponSpeed * 0.25f };

	LimitSpeed( cmd, optSpeed );

	return true;
}

void CMisc::LimitSpeed( CUserCmd& cmd, float maxSpeed ) {
	const auto& velocity{ ctx.m_pLocal->m_vecVelocity( ) };
	const auto speed{ velocity.Length2D( ) };

	Vector forward{ }, right{ };
	Math::AngleVectors( cmd.viewAngles, &forward, &right );

	auto wishSpeed{ maxSpeed / ctx.m_pLocal->m_surfaceFriction( ) };// good idea?
	Vector velDir{ forward.x * cmd.flForwardMove + right.x * cmd.flSideMove,
		forward.y * cmd.flForwardMove + right.y * cmd.flSideMove };

	if ( !cmd.flForwardMove && !cmd.flSideMove ) {
		velDir = { cmd.viewAngles.x, cmd.viewAngles.y, 0.f };
		wishSpeed = 0;
	}

	if ( speed > maxSpeed + 1.f ) {
		wishSpeed = ( speed - maxSpeed ) / ctx.m_pLocal->m_surfaceFriction( );// good idea?
		if ( wishSpeed > 45 )
			wishSpeed = 450.f;

		velDir = velocity;
	}

	cmd.flForwardMove = wishSpeed;
	cmd.flSideMove = 0;

	auto moveDir{ cmd.viewAngles };

	const auto direction{ std::atan2( velDir.y, velDir.x ) };
	moveDir.y = std::remainderf( RAD2DEG( direction ) + ( speed > maxSpeed + 1.f ? 180.f : 0.f ), 360.f );
	MoveMINTFix( cmd, moveDir, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
}

void CMisc::QuickStop( CUserCmd& cmd ) {
	if ( Config::Get<bool>( Vars.MiscQuickStop ) 
		&& !cmd.flForwardMove && !cmd.flSideMove 
		&& !( cmd.iButtons & IN_JUMP ) 
		&& ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND ) {

		if ( ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 20 )
			LimitSpeed( cmd, 0 );
	}
}

//0x224
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
	if ( cmd.viewAngles == wish_angles )
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
}

bool CMisc::MicroMove( CUserCmd& cmd ) {
	if ( !Config::Get<bool>( Vars.AntiaimEnable )
		|| !Config::Get<bool>( Vars.AntiaimDesync )
		|| cmd.iButtons & IN_JUMP
		|| !( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND ) )
		return false;

	if ( ctx.m_pLocal->m_vecVelocity( ).Length2DSqr( ) > 2.f )
		return false;

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

	auto vel = ctx.m_pLocal->m_vecVelocity( );

	if ( vel.Length2D( ) < 2.f
		&& !cmd.flForwardMove && !cmd.flSideMove )
		return;

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled )
		return;

	auto speed2d = vel.Length2D( );
	auto ideal_rot = std::min( RAD2DEG( std::asinf( 15.f / speed2d ) ), 90.f );
	auto sign = cmd.iCommandNumber % 2 ? 1.f : -1.f;

	bool move_forward = cmd.iButtons & IN_FORWARD, move_backward = cmd.iButtons & IN_BACK;
	bool move_left = cmd.iButtons & IN_MOVELEFT, move_right = cmd.iButtons & IN_MOVERIGHT;

	cmd.flForwardMove = speed2d > 0.1f ? 0.f : 450.f;

	if ( move_forward )
		MovementAngle.y += move_left ? 45.f : move_right ? -45.f : 0.f;
	else if ( move_backward )
		MovementAngle.y += move_left ? 135.f : move_right ? -135.f : 180.f;
	else if ( move_left || move_right )
		MovementAngle.y += move_left ? 90.f : -90.f;

	static auto old_yaw = 0.f;
	auto yaw_delta = std::remainder( MovementAngle.y - old_yaw, 360.f ), abs_yaw_delta = std::abs( yaw_delta );
	old_yaw = MovementAngle.y;

	if ( yaw_delta > 0.f ) cmd.flSideMove = -450.f;
	else if ( yaw_delta < 0.f ) cmd.flSideMove = 450.f;

	if ( abs_yaw_delta <= ideal_rot || abs_yaw_delta >= 30.f ) {
		const auto vel_ang = RAD2DEG( std::atan2( vel.y, vel.x ) );
		const auto vel_delta = std::remainder( MovementAngle.y - vel_ang, 360.f );

		auto retrack_speed = ideal_rot * ( ( Config::Get<int>( Vars.MiscAutostrafeSpeed ) / 100.f ) * 3 );

		if ( vel_delta <= retrack_speed || speed2d <= 15.f ) {
			if ( -retrack_speed <= vel_delta || speed2d <= 15.f ) {
				MovementAngle.y += ideal_rot * sign;
				cmd.flSideMove = sign * 450.f;
			}
			else {
				MovementAngle.y = vel_ang - retrack_speed;
				cmd.flSideMove = 450.f;
			}
		}
		else {
			MovementAngle.y = vel_ang + retrack_speed;
			cmd.flSideMove = -450.f;
		}
	}

	MoveMINTFix( cmd, MovementAngle, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
}

void CMisc::SlowWalk( CUserCmd& cmd ) {
	if ( !ctx.m_pWeaponData )
		return;

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled ) {
		cmd.iButtons &= ~IN_WALK;

		const float opt_speed = ( ctx.m_pLocal->m_bIsScoped( ) ? ctx.m_pWeaponData->flMaxSpeedAlt : ctx.m_pWeaponData->flMaxSpeed ) / 3.f;
		const float movement_speed = std::sqrtf( cmd.flSideMove * cmd.flSideMove ) + ( cmd.flForwardMove * cmd.flForwardMove ) + ( cmd.flUpMove * cmd.flUpMove );
		float speed = ctx.m_pLocal->m_vecVelocity( ).Length2D( );

		LimitSpeed( cmd, opt_speed );
	}
}

void CMisc::FakeDuck( CUserCmd& cmd ) {
	const auto Prev = ctx.m_bFakeDucking;

	ctx.m_bFakeDucking = Config::Get<bool>( Vars.MiscFakeDuck ) && Config::Get<keybind_t>( Vars.MiscFakeDuckKey ).enabled && ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND && int( 1.0f / Interfaces::Globals->flIntervalPerTick ) == 64;
	if ( ctx.m_bFakeDucking )
		cmd.iButtons |= IN_BULLRUSH;

	if ( !Prev ) {
		if ( !ctx.m_bFakeDucking )
			return;

		if ( ctx.m_bSendPacket )
			cmd.iButtons &= ~IN_DUCK;
		else
			ctx.m_bSendPacket = true;

		return;
	}

	if ( Interfaces::ClientState->nChokedCommands >= 8 )
		cmd.iButtons |= IN_DUCK;
	else
		cmd.iButtons &= ~IN_DUCK;

	ctx.m_bSendPacket = Interfaces::ClientState->nChokedCommands >= 15;
}

void CMisc::AutoPeek( CUserCmd& cmd ) {
	if ( !Config::Get<bool>( Vars.MiscAutoPeek ) 
		|| !ctx.m_pWeapon
		|| ctx.m_pWeapon->IsKnife( ) || ctx.m_pWeapon->IsGrenade( )
		|| ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER )
		return;

	const auto origin = ctx.m_pLocal->m_vecOrigin( );

	if ( Config::Get<keybind_t>( Vars.MiscAutoPeekKey ).enabled ) {
		if ( !AutoPeeking ) {
			ShouldRetract = false;
			OldOrigin = ctx.m_pLocal->m_vecOrigin( );

			Ray_t ray{ OldOrigin, OldOrigin - Vector( 0.0f, 0.0f, 1000.0f ) };
			CTraceFilter filter{ ctx.m_pLocal, TRACE_EVERYTHING };
			CGameTrace trace;

			Interfaces::EngineTrace->TraceRay( ray, MASK_SOLID, &filter, &trace );

			if ( trace.flFraction < 1.0f )
				OldOrigin = trace.vecEnd + Vector( 0.0f, 0.0f, 2.0f );
		}
		if ( ShouldRetract ) {
			const auto angle{ Math::CalcAngle( ctx.m_pLocal->m_vecOrigin( ), OldOrigin ) };
			MovementAngle.y = angle.y;

			cmd.flForwardMove = 450.f;

			if ( origin.DistTo( OldOrigin ) < 5.f && ctx.m_bCanShoot )
				ShouldRetract = false;

			cmd.flSideMove = 0.f;

			// think of a better way to fix overshooting this is ass
			if ( origin.DistTo2D( OldOrigin ) < ctx.m_pLocal->m_vecVelocity( ).Length2D( ) / 7.66666666667f )
				cmd.flForwardMove = 0;
			else
				MoveMINTFix( cmd, MovementAngle, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
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

	//if ( ctx.m_pLocal->IsDead( ) )
	//	ctx.m_pLocal->m_iObserverMode( ) = 5;

	Vector camForward;

	QAngle camAngles;
	Interfaces::Engine->GetViewAngles( camAngles );

	Math::AngleVectors( camAngles, &camForward, 0, 0 );

	camAngles.z = Config::Get<int>( Vars.VisThirdPersonDistance );

	const auto eyeorigin{ ctx.m_pLocal->GetAbsOrigin( ) + ( ctx.m_bFakeDucking ? Vector( 0, 0, Interfaces::GameMovement->GetPlayerViewOffset( false ).z ) : ctx.m_pLocal->m_vecViewOffset( ) ) };

	const auto vecCamOffset( eyeorigin - ( camForward * camAngles.z ) );

	const Ray_t ray{ eyeorigin, vecCamOffset, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) };
	CGameTrace tr;
	CTraceFilter filter{ ctx.m_pLocal, TRACE_WORLD_ONLY };

	Interfaces::EngineTrace->TraceRay( ray, MASK_NPCWORLDSTATIC, &filter, &tr );

	TPFrac = std::max( Math::Interpolate( TPFrac, tr.flFraction, Interfaces::Globals->flFrameTime * 10.f ), 0.125f );

	camAngles.z *= TPFrac;

	Interfaces::Input->vecCameraOffset.x = camAngles.x;
	Interfaces::Input->vecCameraOffset.y = camAngles.y;
	Interfaces::Input->vecCameraOffset.z = camAngles.z;
}

void CMisc::PlayerMove( ExtrapolationData_t& data ) {
	if ( !( data.m_iFlags & FL_ONGROUND ) ) {
		if ( !Offsets::Cvars.sv_enablebunnyhopping->GetBool( ) ) {
			const auto speed{ data.m_vecVelocity.Length( ) };

			const auto maxSpeed{ data.m_pPlayer->m_flMaxSpeed( ) * 1.1f };
				if ( maxSpeed > 0.f
					&& speed > maxSpeed )
					data.m_vecVelocity *= ( maxSpeed / speed );
		}

		if ( data.m_bWasInAir )
			data.m_vecVelocity.z = Offsets::Cvars.sv_jump_impulse->GetFloat( );
	}
	else
		data.m_vecVelocity.z -=
		Offsets::Cvars.sv_gravity->GetFloat( ) * Interfaces::Globals->flIntervalPerTick;

	CGameTrace trace{ };
	CTraceFilter traceFilter{ nullptr, TRACE_WORLD_ONLY };

	Interfaces::EngineTrace->TraceRay(
		{
			data.m_vecOrigin,
			data.m_vecOrigin + data.m_vecVelocity * Interfaces::Globals->flIntervalPerTick,
			data.m_vecMins, data.m_vecMaxs
		},
		CONTENTS_SOLID, &traceFilter, &trace
	);

	if ( trace.flFraction != 1.f ) {
		for ( int i{ }; i < 2; ++i ) {
			data.m_vecVelocity -= trace.plane.vecNormal * data.m_vecVelocity.DotProduct( trace.plane.vecNormal );

			const auto adjust = data.m_vecVelocity.DotProduct( trace.plane.vecNormal );
			if ( adjust < 0.f )
				data.m_vecVelocity -= trace.plane.vecNormal * adjust;

			Interfaces::EngineTrace->TraceRay(
				{
					trace.vecEnd,
					trace.vecEnd + ( data.m_vecVelocity * ( Interfaces::Globals->flIntervalPerTick * ( 1.f - trace.flFraction ) ) ),
					data.m_vecMins, data.m_vecMaxs
				},
				CONTENTS_SOLID, &traceFilter, &trace
			);

			if ( trace.flFraction == 1.f )
				break;
		}
	}

	data.m_vecOrigin = trace.vecEnd;

	Interfaces::EngineTrace->TraceRay(
		{
			trace.vecEnd,
		{ trace.vecEnd.x, trace.vecEnd.y, trace.vecEnd.z - 2.f },
		data.m_vecMins, data.m_vecMaxs
		},
		CONTENTS_SOLID, &traceFilter, &trace
	);

	data.m_iFlags &= ~FL_ONGROUND;

	if ( trace.flFraction != 1.f
		&& trace.plane.vecNormal.z > 0.7f )
		data.m_iFlags |= FL_ONGROUND;
}

bool CMisc::InPeek( CUserCmd& cmd ) {
	if ( ctx.m_pLocal->m_vecVelocity( ).Length( ) < 1.2f ) {
		Features::Exploits.m_bAlreadyPeeked = false;
		return false;
	}

	matrix3x4_t backupMatrix[ 256 ];
	std::memcpy( backupMatrix, ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto backupAbsOrigin{ ctx.m_pLocal->GetAbsOrigin( ) };

	const auto backupState{ *ctx.m_pLocal->m_pAnimState( ) };
	CAnimationLayer backupLayers[ 13 ]{ };
	std::memcpy( backupLayers, ctx.m_pLocal->m_AnimationLayers( ), 13 * sizeof CAnimationLayer );
	const auto backupPose{ ctx.m_pLocal->m_flPoseParameter( ) };

	const auto backupTickbase{ ctx.m_pLocal->m_nTickBase( ) };
	const auto backupFlags{ ctx.m_pLocal->m_fFlags( ) };
	const auto backupVel{ ctx.m_pLocal->m_vecAbsVelocity( ) };

	const auto backupJumping{ Features::AnimSys.m_bJumping };
	const auto backupLowerBodyRealignTimer{ Features::AnimSys.m_flLowerBodyRealignTimer };

	ExtrapolationData_t extrapolated{ ctx.m_pLocal };
	for ( int i{ }; i < 2; ++i ) {
		PlayerMove( extrapolated );

		ctx.m_pLocal->m_fFlags( ) = extrapolated.m_iFlags;
		ctx.m_pLocal->m_vecAbsVelocity( ) = extrapolated.m_vecVelocity;

		ctx.m_pLocal->m_nTickBase( )++;

		Features::AnimSys.UpdateLocal( cmd.viewAngles, true, cmd );
	}

	ctx.m_pLocal->m_nTickBase( ) = backupTickbase;
	ctx.m_pLocal->m_fFlags( ) = backupFlags;
	ctx.m_pLocal->m_vecAbsVelocity( ) = backupVel;

	Features::AnimSys.m_bJumping = backupJumping;
	Features::AnimSys.m_flLowerBodyRealignTimer = backupLowerBodyRealignTimer;

	ctx.m_pLocal->SetAbsOrigin( extrapolated.m_vecOrigin );

	Features::AnimSys.SetupBonesRebuilt( ctx.m_pLocal, ( matrix3x4a_t* )( ctx.m_pLocal->m_CachedBoneData( ).Base( ) ), 
		BONE_USED_BY_HITBOX, ctx.m_flFixedCurtime, true );

	*ctx.m_pLocal->m_pAnimState( ) = backupState;
	std::memcpy( ctx.m_pLocal->m_AnimationLayers( ), backupLayers, 13 * sizeof CAnimationLayer );
	ctx.m_pLocal->m_flPoseParameter( ) = backupPose;

	const auto hitboxSet{ ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return false;

	bool damageable{ };

	std::vector<CBasePlayer*> enemies{ };

	for ( auto i = 1; i <= 64; ++i ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player
			|| !player->IsPlayer( )
			|| player->Dormant( )
			|| player->IsDead( )
			|| player->IsTeammate( )
			|| !player->GetWeapon( )
			|| !player->GetWeapon( )->GetCSWeaponData( ) )
			continue;

		enemies.push_back( player );
	}

	auto orig = ctx.m_pLocal->GetAbsOrigin( );
	std::sort( enemies.begin( ), enemies.end( ), [ orig ]( CBasePlayer* pl, CBasePlayer* pl0 ) { return orig.DistTo( pl->m_vecOrigin( ) ) < orig.DistTo( pl0->m_vecOrigin( ) ); } );

	for ( auto& player : enemies ) {
		auto enemyShootPos{ player->m_vecOrigin( ) };
		enemyShootPos.z += ( player->m_fFlags( ) & FL_DUCKING ) ? 46.f : 64.f;

		for ( const auto& hb : { HITBOX_CHEST, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {//HITBOX_RIGHT_THIGH, HITBOX_LEFT_THIGH
			const auto hitbox{ hitboxSet->GetHitbox( hb ) };
			if ( !hitbox )
				continue;

			const auto center{ Math::VectorTransform( ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f,
				ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] ) };

			const auto data{ Features::Autowall.FireBullet( player, ctx.m_pLocal, player->GetWeapon( )->GetCSWeaponData( ),
				player->GetWeapon( )->m_iItemDefinitionIndex( ) == WEAPON_TASER, enemyShootPos, center, true ) };

			if ( data.dmg > 0 ) {
				damageable = true;
				break;
			}
		}

		if ( damageable )
			break;
	}

	ctx.m_pLocal->SetAbsOrigin( backupAbsOrigin );
	memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), backupMatrix, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	if ( !damageable )
		Features::Exploits.m_bAlreadyPeeked = false;

	return damageable;
}

bool CMisc::IsDefensivePositionHittable( ) {
	const auto backupOrigin{ ctx.m_pLocal->GetAbsOrigin( ) };
	ctx.m_pLocal->SetAbsOrigin( ctx.m_vecSetupBonesOrigin );
	std::memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_matRealLocalBones, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	const auto hitboxSet{ ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) ) };
	if ( !hitboxSet )
		return false;

	bool damageable{ };

	std::vector<CBasePlayer*> enemies{ };

	for ( auto i = 1; i <= 64; ++i ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player
			|| !player->IsPlayer( )
			|| player->Dormant( )
			|| player->IsDead( )
			|| player->IsTeammate( )
			|| !player->GetWeapon( )
			|| !player->GetWeapon( )->GetCSWeaponData( ) )
			continue;

		enemies.push_back( player );
	}

	auto orig = ctx.m_pLocal->GetAbsOrigin( );
	std::sort( enemies.begin( ), enemies.end( ), [ orig ]( CBasePlayer* pl, CBasePlayer* pl0 ) { return orig.DistTo( pl->m_vecOrigin( ) ) < orig.DistTo( pl0->m_vecOrigin( ) ); } );

	for ( auto& player : enemies ) {
		auto enemyShootPos{ player->m_vecOrigin( ) };
		enemyShootPos.z += ( player->m_fFlags( ) & FL_DUCKING ) ? 46.f : 64.f;

		for ( const auto& hb : { HITBOX_CHEST, HITBOX_RIGHT_UPPER_ARM, HITBOX_LEFT_UPPER_ARM, HITBOX_RIGHT_FOOT, HITBOX_LEFT_FOOT } ) {//HITBOX_RIGHT_THIGH, HITBOX_LEFT_THIGH
			const auto hitbox{ hitboxSet->GetHitbox( hb ) };

			const auto center{ Math::VectorTransform( ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f,
				ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] ) };

			const auto data{ Features::Autowall.FireBullet( player, ctx.m_pLocal, player->GetWeapon( )->GetCSWeaponData( ),
				player->GetWeapon( )->m_iItemDefinitionIndex( ) == WEAPON_TASER, enemyShootPos, center, true ) };

			if ( data.dmg > 0 ) {
				damageable = true;
				break;
			}
		}

		if ( damageable )
			break;
	}

	return damageable;

	ctx.m_pLocal->SetAbsOrigin( backupOrigin );
}

void CMisc::UpdateIncomingSequences( INetChannel* pNetChannel ) {
	if ( pNetChannel == nullptr )
		return;

	// set to real sequence to update, otherwise needs time to get it work again
	if ( !m_nLastIncomingSequence )
		m_nLastIncomingSequence = pNetChannel->iInSequenceNr;

	// check how much sequences we can spike
	if ( pNetChannel->iInSequenceNr > m_nLastIncomingSequence ) {
		m_nLastIncomingSequence = pNetChannel->iInSequenceNr;
		m_vecSequences.emplace_front( SequenceObject_t( pNetChannel->iInReliableState, pNetChannel->iOutReliableState, pNetChannel->iInSequenceNr, Interfaces::Globals->flRealTime ) );
	}

	// is cached too much sequences
	if ( m_vecSequences.size( ) > 2048U )
		m_vecSequences.pop_back( );
}

void CMisc::ClearIncomingSequences( ) {
	if ( !m_vecSequences.empty( ) ) {
		m_nLastIncomingSequence = 0;
		m_vecSequences.clear( );
	}
}

void CMisc::AddLatencyToNetChannel( INetChannel* pNetChannel, float flLatency ) {
	for ( const auto& sequence : m_vecSequences ) {
		if ( Interfaces::Globals->flRealTime - sequence.flCurrentTime >= flLatency ) {
			pNetChannel->iInReliableState = sequence.iInReliableState;
			pNetChannel->iInSequenceNr = sequence.iSequenceNr;
			break;
		}
	}
}