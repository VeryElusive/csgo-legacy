#include "misc.h"
#include "../rage/ragebot.h"

// this file was way too long so i moved to it's own file.

//https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp

bool CMisc::AutoStop( CUserCmd& cmd ) {
	if ( !Features::Ragebot.m_bShouldStop )
		return false;

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
	const auto optSpeed{ maxWeaponSpeed / 3.f };

	const auto& velocity{ ctx.m_pLocal->m_vecVelocity( ) };
	const auto speed{ velocity.Length2D( ) };

	auto& side{ cmd.flSideMove }, & forward{ cmd.flForwardMove };

	if ( optSpeed >= speed )
		LimitSpeed( cmd, optSpeed );
	else {
		cmd.iButtons |= 0x20000u;
		auto finalWishSpeed = std::min( maxWeaponSpeed, 250.f );

		const auto ducking =
			cmd.iButtons & IN_DUCK
			|| ctx.m_pLocal->m_flDuckAmount( )
			|| ctx.m_pLocal->m_fFlags( ) & FL_DUCKING;

		bool slowDownBro{ };
		if ( ctx.m_pWeapon
			&& Offsets::Cvars.sv_accelerate->GetBool( ) ) {
			const auto item_index = static_cast< std::uint16_t >( ctx.m_pWeapon->m_iItemDefinitionIndex( ) );
			if ( ctx.m_pWeapon->m_zoomLevel( ) > 0
				&& ( item_index == 11 || item_index == 38 || item_index == 9 || item_index == 8 || item_index == 39 || item_index == 40 ) )
				slowDownBro = ( maxWeaponSpeed * 0.52f ) < 110.f;

			if ( !ducking
				|| slowDownBro )
				finalWishSpeed *= std::min( 1.f, maxWeaponSpeed / 250.f );
		}

		if ( ducking
			&& !slowDownBro )
			finalWishSpeed /= 3.f;

		finalWishSpeed =
			( ( Interfaces::Globals->flIntervalPerTick * Offsets::Cvars.sv_accelerate->GetFloat( ) ) * finalWishSpeed )
			* ctx.m_pLocal->m_surfaceFriction( );

		if ( optSpeed <= ( speed - finalWishSpeed ) ) {
			if ( finalWishSpeed <= 0.f ) {
				LimitSpeed( cmd, optSpeed );

				return true;
			}
		}
		else {
			finalWishSpeed = speed - optSpeed;

			if ( ( speed - optSpeed ) < 0.f ) {
				LimitSpeed( cmd, optSpeed );

				return true;
			}
		}

		QAngle dir;
		Math::VectorAngles( velocity * -1, dir );

		dir.y = cmd.viewAngles.y - dir.y;

		Vector move;
		Math::AngleVectors( dir, &move );

		cmd.flForwardMove = std::copysign( maxWeaponSpeed, move.x ) * finalWishSpeed;
		cmd.flSideMove = std::copysign( maxWeaponSpeed, move.y ) * finalWishSpeed;
	}

	return true;
}

void CMisc::LimitSpeed( CUserCmd& cmd, float maxSpeed ) {
	const auto cmdSpeed{ ( cmd.flSideMove * cmd.flSideMove ) + ( cmd.flForwardMove * cmd.flForwardMove ) + ( cmd.flUpMove * cmd.flUpMove ) };

	Vector fwd{ }, right{ };

	Math::AngleVectors( cmd.viewAngles, &fwd, &right );

	if ( cmdSpeed > ( maxSpeed * maxSpeed ) ) {
		cmd.flSideMove *= maxSpeed / std::sqrt( cmdSpeed );
		cmd.flForwardMove *= maxSpeed / std::sqrt( cmdSpeed );
		cmd.flUpMove *= maxSpeed / std::sqrt( cmdSpeed );
	}

	FullWalkMoveRebuild( cmd, fwd, right, ctx.m_pLocal->m_vecVelocity( ), maxSpeed );
}

void CMisc::FullWalkMoveRebuild( CUserCmd& user_cmd, Vector& fwd, Vector& right, Vector& velocity, float maxSpeed ) {
	if ( ctx.m_pLocal->m_hGroundEntity( ) != -1 ) {
		velocity.z = 0.f;

		const auto speed{ velocity.Length( ) };

		// apply ground friction
		// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp#L1633
		if ( speed >= 0.1f ) {
			const auto friction{ Offsets::Cvars.sv_friction->GetFloat( ) * ctx.m_pLocal->m_surfaceFriction( ) };

			const auto sv_stopspeed{ Offsets::Cvars.sv_stopspeed->GetFloat( ) };
			const auto control{ speed < sv_stopspeed ? sv_stopspeed : speed };

			const auto new_speed = std::max( 0.f, speed - ( ( control * friction ) * Interfaces::Globals->flIntervalPerTick ) );
			if ( speed != new_speed )
				velocity *= new_speed / speed;
		}

		WalkMoveRebuild( user_cmd, fwd, right, velocity, maxSpeed );

		velocity.z = 0.f;
	}

	const auto sv_maxvelocity = Offsets::Cvars.sv_maxvelocity->GetFloat( );
	for ( std::size_t i{ }; i < 3u; ++i ) {
		auto& element = velocity[ i ];

		if ( element > sv_maxvelocity )
			element = sv_maxvelocity;
		else if ( element < -sv_maxvelocity )
			element = -sv_maxvelocity;
	}
}


void CMisc::WalkMoveRebuild( CUserCmd& user_cmd, Vector& fwd, Vector& right, Vector& velocity, float maxSpeed ) {
	if ( fwd.z != 0.f )
		fwd.NormalizeInPlace( );

	if ( right.z != 0.f )
		right.NormalizeInPlace( );

	Vector wishvel{
		fwd.x * user_cmd.flForwardMove + right.x * user_cmd.flSideMove,
		fwd.y * user_cmd.flForwardMove + right.y * user_cmd.flSideMove,
		0.f
	};

	auto wishdir = wishvel;

	auto wishspeed = wishdir.NormalizeInPlace( );
	if ( wishspeed
		&& wishspeed > maxSpeed ) {
		wishvel *= maxSpeed / wishspeed;

		wishspeed = maxSpeed;
	}

	velocity.z = 0.f;
	AccelerateRebuild( user_cmd, wishdir, wishspeed, velocity, Offsets::Cvars.sv_accelerate->GetFloat( ), maxSpeed );
	velocity.z = 0.f;

	const auto speed_sqr = velocity.LengthSqr( );
	if ( speed_sqr > ( maxSpeed * maxSpeed ) )
		velocity *= maxSpeed / std::sqrt( speed_sqr );

	if ( velocity.Length( ) < 1.f )
		velocity = { 0,0,0 };
}

void CMisc::AccelerateRebuild( CUserCmd& user_cmd, const Vector& wishdir, const float wishspeed, Vector& velocity, float acceleration, float maxSpeed ) {
	const auto cur_speed = velocity.DotProduct( wishdir );

	const auto add_speed = wishspeed - cur_speed;
	if ( add_speed <= 0.f )
		return;

	const auto v57 = std::max( cur_speed, 0.f );

	const auto ducking =
		user_cmd.iButtons & IN_DUCK
		|| ctx.m_pLocal->m_flDuckAmount( )
		|| ctx.m_pLocal->m_fFlags( ) & FL_DUCKING;

	auto v20 = true;
	if ( ducking
		|| !( user_cmd.iButtons & IN_SPEED ) )
		v20 = false;

	auto finalwishspeed = std::max( wishspeed, 250.f );
	auto abs_finalwishspeed = finalwishspeed;

	bool slow_down_to_fast_nigga{ };

	if ( ctx.m_pWeapon
		&& Offsets::Cvars.sv_accelerate_use_weapon_speed->GetBool( ) ) {
		const auto item_index = static_cast< std::uint16_t >( ctx.m_pWeapon->m_iItemDefinitionIndex( ) );
		if ( ctx.m_pWeapon->m_zoomLevel( ) > 0
			&& ( item_index == 11 || item_index == 38 || item_index == 9 || item_index == 8 || item_index == 39 || item_index == 40 ) )
			slow_down_to_fast_nigga = ( maxSpeed * 0.52f ) < 110.f;

		const auto modifier = std::min( 1.f, maxSpeed / 250.f );

		abs_finalwishspeed *= modifier;

		if ( ( !ducking && !v20 )
			|| slow_down_to_fast_nigga )
			finalwishspeed *= modifier;
	}

	if ( ducking ) {
		if ( !slow_down_to_fast_nigga )
			finalwishspeed *= 0.34f;

		abs_finalwishspeed *= 0.34f;
	}

	if ( v20 ) {
		if ( !slow_down_to_fast_nigga )
			finalwishspeed *= 0.52f;

		abs_finalwishspeed *= 0.52f;

		const auto abs_finalwishspeed_minus5 = abs_finalwishspeed - 5.f;
		if ( v57 < abs_finalwishspeed_minus5 ) {
			const auto v30 =
				std::max( v57 - abs_finalwishspeed_minus5, 0.f )
				/ std::max( abs_finalwishspeed - abs_finalwishspeed_minus5, 0.f );

			const auto v27 = 1.f - v30;
			if ( v27 >= 0.f )
				acceleration = std::min( v27, 1.f ) * acceleration;
			else
				acceleration = 0.f;
		}
	}

	const auto v33 = std::min(
		add_speed,
		( ( Interfaces::Globals->flIntervalPerTick * acceleration ) * finalwishspeed )
		* ctx.m_pLocal->m_surfaceFriction( )
	);

	velocity += wishdir * v33;

	const auto len = velocity.Length( );
	if ( len
		&& len > maxSpeed )
		velocity *= maxSpeed / len;

}