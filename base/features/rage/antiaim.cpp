#include "antiaim.h"
#include "../animations/animation.h"
#include "../rage/exploits.h"

void CAntiAim::Pitch( CUserCmd& cmd ) {
	if ( Condition( cmd ) )
		return;

	if ( Interfaces::ClientState->nChokedCommands < 1 && Config::Get<bool>( Vars.AntiaimDesync ) )
		ctx.m_bSendPacket = false;

	switch ( Config::Get<int>( Vars.AntiaimPitch ) ) {
	case 1: cmd.viewAngles.x = -89.f; break;// up
	case 2: cmd.viewAngles.x =  89.f; break;// down
	case 3: cmd.viewAngles.x =  0.f;  break;// zero
	default: break;
	}
}

void CAntiAim::PickYaw( float& yaw ) {
	static bool Inverted{ };
	static int RotatedYaw{ 180 };

	if ( !Interfaces::ClientState->nChokedCommands )
		ChokeCycleJitter = !ChokeCycleJitter;

	const int& YawRange{ Config::Get<int>( Vars.AntiaimYawRange ) };

	switch ( Config::Get<int>( Vars.AntiaimYaw ) ) {
	case 0: yaw += 0.f; break;// forward
	case 1: yaw += 180.f; break;// backward
	case 2: {// rotate
		RotatedYaw -= Inverted ? Config::Get<int>( Vars.AntiaimYawSpeed ) : -Config::Get<int>( Vars.AntiaimYawSpeed );

		if ( RotatedYaw < 180 - ( YawRange * 0.5f ) )
			Inverted = false;
		else if ( RotatedYaw > 180 + ( YawRange * 0.5f ) )
			Inverted = true;

		RotatedYaw = std::clamp<int>( RotatedYaw, YawRange * -0.5f, YawRange * 0.5f );
		
		yaw += RotatedYaw;
	}break;
	case 3: {// jitter 
		yaw += 180.f + YawRange * ( ChokeCycleJitter ? 0.5f : -0.5f );
	}break;
	default: break;
	}
}
float CAntiAim::BaseYaw( CUserCmd& cmd ) {
	if ( Config::Get<keybind_t>( Vars.AntiaimInvertSpam ).enabled )
		Invert = ChokeCycleJitter;
	else
		Invert = Config::Get<keybind_t>( Vars.AntiaimInvert ).enabled;

	auto yaw = cmd.viewAngles.y;

	if ( Config::Get<bool>( Vars.AntiAimManualDir ) ) {
		if ( ManualSide == 1 ) {
			yaw += 90.f;
			return yaw;
		}
		else if ( ManualSide == 2 ) {
			yaw -= 90.f;
			return yaw;
		}
	}

	AtTarget( yaw );
	PickYaw( yaw );
	//AutoDirection( yaw );

	return yaw;
}

void CAntiAim::AtTarget( float& yaw ) {
	CBasePlayer* bestPlayer{ nullptr };
	auto bestValue = INT_MAX;

	if ( !Config::Get<int>( Vars.AntiaimAtTargets ) && !Config::Get<bool>( Vars.AntiaimAntiBackStab ) )
		return;

	for ( auto i = 1; i <= 64; ++i ) {
		const auto player{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !player
			|| !player->IsPlayer( )
			|| player->Dormant( )
			|| player->IsDead( )
			|| player->IsTeammate( ) )
			continue;

		const auto dist{ ( ctx.m_pLocal->m_vecOrigin( ) - player->m_vecOrigin( ) ).Length( ) };
		if ( Config::Get<bool>( Vars.AntiaimAntiBackStab ) ) {
			if ( player->GetWeapon( )->IsKnife( ) ) {
				if ( dist < 250 ) {
					bestPlayer = player;
					break;
				}
			}
		}

		switch ( Config::Get<int>( Vars.AntiaimAtTargets ) ) {
		case 1: {// FOV/closest to crosshair
			const auto fov = Math::GetFov( ctx.m_angOriginalViewangles, Math::CalcAngle( ctx.m_vecEyePos, player->GetAbsOrigin( ) ) );
			if ( fov > bestValue )
				continue;

			bestValue = fov;
			bestPlayer = player;
		}break;
		case 2: {// Distance
			if ( dist > bestValue )
				continue;

			bestValue = dist;
			bestPlayer = player;
		} break;
		default: break;
		}
	}

	if ( !bestPlayer )
		return;

	const auto x{ bestPlayer->m_vecOrigin( ).x - ctx.m_pLocal->m_vecOrigin( ).x };
	const auto y{ bestPlayer->m_vecOrigin( ).y - ctx.m_pLocal->m_vecOrigin( ).y };

	yaw = x == 0.f && y == 0.f ? 0.f : RAD2DEG( std::atan2( y, x ) );
}

bool CAntiAim::Condition( CUserCmd& cmd ) {
	if ( ctx.m_pLocal->m_MoveType( ) == MOVETYPE_NOCLIP || ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER )
		return true;

	if ( !Config::Get<bool>( Vars.AntiaimEnable ) )
		return true;

	if ( !ctx.m_pWeapon )
		return true;

	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) )
		return true;

	if ( cmd.iButtons & IN_ATTACK && !ctx.m_pWeapon->IsGrenade( ) )
		return true;

	if ( ctx.m_pWeapon->IsGrenade( ) && ctx.m_pWeapon->m_fThrowTime( ) )
		return true;

	// e
	if ( cmd.iButtons & IN_USE )
		return true;

	// right click
	if ( cmd.iButtons & IN_ATTACK2 && ctx.m_pWeapon->IsKnife( ) /*&& ctx.can_shoot*/ )
		return true;

	return false;
}

void CAntiAim::FakeLag( ) {
	if ( !Config::Get<int>( Vars.AntiaimFakeLagLimit ) && !Config::Get<bool>( Vars.AntiaimEnable ) )
		return;

	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) )
		return;

	static int maxChoke = Config::Get<int>( Vars.AntiaimFakeLagLimit );

	const int& max = Config::Get<int>( Vars.AntiaimFakeLagLimit );

	if ( !Interfaces::ClientState->nChokedCommands )
		maxChoke = Math::RandomInt( static_cast< int >( max * ( 1.f - ( static_cast< float >( Config::Get<int>( Vars.AntiaimFakeLagVariance ) ) / 100.f ) ) ), max );

	if ( Interfaces::Engine->IsVoiceRecording( ) )
		maxChoke = 1;

	const auto& localData = ctx.m_cLocalData.at( Interfaces::ClientState->iLastOutgoingCommand % 150 );

	ctx.m_bSendPacket = Interfaces::ClientState->nChokedCommands >= maxChoke;

	if ( Config::Get<bool>( Vars.AntiaimFakeLagBreakLC )
		&& ( ctx.m_pLocal->m_vecOrigin( ) - localData.PredictedNetvars.m_vecOrigin ).LengthSqr( ) > 4096.f )
		ctx.m_bSendPacket = true;
}

void CAntiAim::RunLocalModifications( CUserCmd& cmd, bool sendPacket ) {
	const float yaw = std::remainderf( BaseYaw( cmd ), 360.f );

	auto i = 1;
	auto chokedCmds = Interfaces::ClientState->nChokedCommands;

	const auto totalCmds = chokedCmds + 1;
	if ( totalCmds < 1
		|| !sendPacket )
		return;

	if ( Condition( cmd ) ) {
		Features::Misc.NormalizeMovement( cmd );

		if ( ctx.m_bFilledAnims ) {
			ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
			ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
			*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;
		}

		Features::AnimSys.UpdateLocal( cmd.viewAngles, false );

		if ( Features::Exploits.m_iShiftAmount
			&& !Features::Exploits.m_bRealCmds )
			return;

		ctx.m_bFilledAnims = true;// !hs;
		memcpy( ctx.m_pAnimationLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
		ctx.m_flPoseParameter = ctx.m_pLocal->m_flPoseParameter( );
		ctx.m_cAnimstate = *ctx.m_pLocal->m_pAnimState( );
		ctx.m_flAbsYaw = ctx.m_cAnimstate.flAbsYaw;
		return;
	}

	if ( ctx.m_bFilledAnims ) {
		ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
		ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
		*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;
	}

	const auto inShot = ctx.m_iLastShotNumber > Interfaces::ClientState->iLastOutgoingCommand
		&& ctx.m_iLastShotNumber <= ( Interfaces::ClientState->iLastOutgoingCommand + Interfaces::ClientState->nChokedCommands + 1 );

	for ( ; i <= totalCmds; ++i, --chokedCmds ) {
		const auto j = ( Interfaces::ClientState->iLastOutgoingCommand + i ) % 150;

		auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
		auto& curLocalData{ ctx.m_cLocalData.at( j ) };

		if ( curLocalData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) )
			continue;

		if ( curUserCmd.iTickCount >= INT_MAX )
			continue;

		if ( curLocalData.PredictedNetvars.m_MoveType != MOVETYPE_LADDER
			&& curLocalData.m_MoveType != MOVETYPE_LADDER
			&& !curLocalData.m_bThrowingNade
			&& !( curUserCmd.iButtons & IN_ATTACK && !ctx.m_pWeapon->IsGrenade( ) ) ) {
			const auto oldViewAngles{ curUserCmd.viewAngles };

			curUserCmd.viewAngles.y = yaw;

			if ( chokedCmds > 0 && !inShot && Config::Get<bool>( Vars.AntiaimDesync ) )
				curUserCmd.viewAngles.y = std::remainderf( curUserCmd.viewAngles.y + ( Invert ? -120.f : 120.f ), 360.f );

			Features::Misc.MoveMINTFix(
				curUserCmd, oldViewAngles,
				curLocalData.PredictedNetvars.m_iFlags,
				curLocalData.PredictedNetvars.m_MoveType
			);
		}

		Features::Misc.NormalizeMovement( curUserCmd );

		ctx.m_pLocal->m_nTickBase( ) = curLocalData.PredictedNetvars.m_nTickBase;
		ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
		ctx.m_pLocal->m_vecVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
		ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecAbsVelocity;

		const auto lastCmd{ curUserCmd.iCommandNumber == cmd.iCommandNumber };
		Features::AnimSys.UpdateLocal( curUserCmd.viewAngles, lastCmd );

		if ( lastCmd )
			continue;

		Interfaces::Input->pVerifiedCommands[ j ].userCmd = curUserCmd;
		Interfaces::Input->pVerifiedCommands[ j ].uHashCRC = curUserCmd.GetChecksum( );
	}

	ctx.m_bFilledAnims = true;

	if ( Features::Exploits.m_iShiftAmount
		&& !Features::Exploits.m_bRealCmds )
		return;

	memcpy( ctx.m_pAnimationLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
	ctx.m_flPoseParameter = ctx.m_pLocal->m_flPoseParameter( );
	ctx.m_cAnimstate = *ctx.m_pLocal->m_pAnimState( );
	ctx.m_flAbsYaw = ctx.m_cAnimstate.flAbsYaw;
}

// pasta reis courtesy of slazy
/*
bool CAntiAim::AutoDirection( float& yaw ) {
	if ( !Config::Get<int>( Vars.AntiaimFreestand ) )
		return false;

	CBasePlayer* best_player{ };
	auto best_fov = std::numeric_limits< float >::max( );

	const auto view_angles = ctx.m_angOriginalViewangles;

	for ( auto i = 1; i <= 64; ++i ) {
		const auto player = static_cast< CBasePlayer* >(
			Interfaces::ClientEntityList->GetClientEntity( i )
			);
		if ( !player
			|| player->Dormant( )
			|| player->IsDead( )
			|| player->IsTeammate( ) )
			continue;

		const auto fov = Math::GetFov( view_angles, Math::CalcAngle( ctx.m_vecEyePos, player->WorldSpaceCenter( ) ) );
		if ( fov >= best_fov )
			continue;

		best_fov = fov;
		best_player = player;
	}

	if ( !best_player )
		return false;

	struct angle_data_t {
		__forceinline constexpr angle_data_t( ) = default;

		__forceinline angle_data_t( const float yaw ) : m_yaw{ yaw } {}

		int		m_dmg{ };
		float	m_yaw{ }, m_dist{ };
		bool	m_can_do_dmg{ };
	};

	std::array< angle_data_t, 3u > angles{
		{
			{ std::remainder( yaw, 360.f ) },
		{ std::remainder( yaw + 90.f, 360.f ) },
		{ std::remainder( yaw - 90.f, 360.f ) }
		}
	};

	constexpr auto k_range = 30.f;

	auto enemy_shoot_pos = best_player->m_vecOrigin( );

	enemy_shoot_pos.z += 64.f;

	bool valid{ };

	const auto& local_shoot_pos = ctx.m_vecEyePos;
	for ( auto& angle : angles ) {
		const auto rad_yaw = DEG2RAD( angle.m_yaw );

		const auto pen_data = Features::Autowall.FireEmulated(
			best_player, ctx.m_pLocal, enemy_shoot_pos,
			{
				local_shoot_pos.x + std::cos( rad_yaw ) * k_range,
				local_shoot_pos.y + std::sin( rad_yaw ) * k_range,
				local_shoot_pos.z
			}
		);

		if ( pen_data.dmg < 1 )
			continue;

		angle.m_dmg = pen_data.dmg;

		angle.m_can_do_dmg = angle.m_dmg > 0;

		if ( !angle.m_can_do_dmg )
			continue;

		valid = true;
	}

	if ( valid ) {
		float best_dmg{ };
		std::size_t best_index{ };

		for ( std::size_t i{ }; i < angles.size( ); ++i ) {
			const auto& angle = angles.at( i );
			if ( !angle.m_can_do_dmg
				|| angle.m_dmg <= best_dmg )
				continue;

			best_dmg = angle.m_dmg;
			best_index = i;
		}

		const auto& best_angle = angles.at( best_index );

		if ( Config::Get<int>( Vars.AntiaimFreestand ) == 2 ) {
			yaw = best_angle.m_yaw;
			return true;
		}
		else {
			const auto diff = Math::AngleDiff( yaw, best_angle.m_yaw );

			Invert = diff >= 0.f;
		}

		return false;
	}

	valid = false;

	constexpr auto k_step = 4.f;

	for ( auto& angle : angles ) {
		const auto rad_yaw = DEG2RAD( angle.m_yaw );

		const Vector dst{
			local_shoot_pos.x + std::cos( rad_yaw ) * k_range,
			local_shoot_pos.y + std::sin( rad_yaw ) * k_range,
			local_shoot_pos.z
		};

		auto dir = dst - enemy_shoot_pos;

		const auto len = dir.NormalizeInPlace( );
		if ( len <= 0.f )
			continue;

		for ( float i{ }; i < len; i += k_step ) {
			const auto contents = Interfaces::EngineTrace->GetPointContents( local_shoot_pos + dir * i, MASK_SHOT_HULL );
			if ( !( contents & MASK_SHOT_HULL ) )
				continue;

			auto mult = 1.f;

			if ( i > ( len * 0.5f ) )
				mult = 1.25f;

			if ( i > ( len * 0.75f ) )
				mult = 1.25f;

			if ( i > ( len * 0.9f ) )
				mult = 2.f;

			angle.m_dist += k_step * mult;

			valid = true;
		}
	}

	if ( !valid )
		return false;

	if ( std::abs( angles.at( 0u ).m_dist - angles.at( 1u ).m_dist ) >= 10.f
		|| std::abs( angles.at( 0u ).m_dist - angles.at( 2u ).m_dist ) >= 10.f ) {
		std::sort(
			angles.begin( ), angles.end( ),
			[ ]( const angle_data_t& a, const angle_data_t& b ) {
				return a.m_dist > b.m_dist;
			}
		);

		const auto& best_angle = angles.front( );
		if ( best_angle.m_dist > 400.f )
			return false;

		if ( Config::Get<int>( Vars.AntiaimFreestand ) == 2 ) {
			yaw = best_angle.m_yaw;
			return true;
		}
		else {
			const auto diff = Math::AngleDiff( yaw, best_angle.m_yaw );

			Invert = diff >= 0.f;
		}

		return false;
	}

	return false;
}*/