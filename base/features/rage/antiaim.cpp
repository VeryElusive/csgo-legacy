#include "antiaim.h"
#include "../animations/animation.h"
#include "../rage/exploits.h"

bool CAntiAim::Pitch( CUserCmd& cmd ) {
	if ( Condition( cmd, true ) )
		return false;

	switch ( Config::Get<int>( Vars.AntiaimPitch ) ) {
	case 1: cmd.viewAngles.x = -89.f; break;// up
	case 2: cmd.viewAngles.x =  89.f; break;// down
	case 3: cmd.viewAngles.x =  0.f;  break;// zero
	case 4: cmd.viewAngles.x = Math::RandomFloat( -89.f, 89 );  break;// random
	default: break;
	}

	return true;
}

void CAntiAim::PickYaw( float& yaw ) {
	static bool invert{ };
	static int rotatedYaw{ };

	const int& yawRange{ Config::Get<int>( Vars.AntiaimYawRange ) };

	switch ( Config::Get<int>( Vars.AntiaimYaw ) ) {
	case 0: yaw += 0.f; break;// forward
	case 1: yaw += 180.f; break;// backward
	case 2: yaw += 90.f; break;// left
	case 3: yaw -= 90.f; break;// right
	}

	switch ( Config::Get<int>( Vars.AntiaimYawAdd ) ) {
	case 1:// jitter 
		yaw += yawRange * ( ChokeCycleJitter ? 0.5f : -0.5f );
		break;
	case 2: {// rotate
		//if ( Interfaces::ClientState->nChokedCommands )
		//	break;

		rotatedYaw -= invert ? Config::Get<int>( Vars.AntiaimYawSpeed ) : -Config::Get<int>( Vars.AntiaimYawSpeed );

		if ( rotatedYaw < yawRange * -0.5f )
			invert = false;
		else if ( rotatedYaw > yawRange * 0.5f )
			invert = true;

		rotatedYaw = std::clamp<int>( rotatedYaw, yawRange * -0.5f, yawRange * 0.5f );
		
		yaw += rotatedYaw;
	}break;
	case 3: {// spin
		rotatedYaw += Config::Get<int>( Vars.AntiaimYawSpeed );
		rotatedYaw = std::remainderf( rotatedYaw, 360.f );
		yaw = rotatedYaw;
		break;
	}
	case 4: {// random
		yaw += Math::RandomFloat( -yawRange / 2.f, yawRange / 2.f );
		break;
	}
	default: break;
	}
}

void CAntiAim::Yaw( CUserCmd& cmd, bool& sendPacket ) {
	cmd.viewAngles.y = BaseYaw( cmd );

	const auto m_flVelocityLengthXY{ ctx.m_pLocal->m_vecVelocity( ).Length2D( ) };

	if ( m_flVelocityLengthXY <= 0.1f ) {
		if ( !sendPacket ) {
			if ( Config::Get<bool>( Vars.AntiaimDesync ) ) {
				switch ( Config::Get<int>( Vars.AntiaimBreakAngle ) ) {
				case 0:// opposite
					cmd.viewAngles.y += 180.f;
					break;
				case 1:// back
					cmd.viewAngles.y = ctx.m_angOriginalViewangles.y + 180.f;
					break;
				default: break;
				}	
			}
		}

		if ( ctx.m_flFixedCurtime > m_flLowerBodyRealignTimer ) {
			cmd.viewAngles.y -= 90.f;
			sendPacket = false;
		}
	}
	
	if ( m_flVelocityLengthXY <= 0.1f ) {
		if ( Config::Get<bool>( Vars.AntiaimDistortion ) ) {
			static bool reRoll{ };
			static float random{ };
			static float cur{ };

			if ( Config::Get<bool>( Vars.AntiaimDistortionSpike ) ) {
				random = Math::RandomFloat( 0, Config::Get<int>( Vars.AntiaimDistortionRange ) );
				cmd.viewAngles.y += random;
			}
			else {
				if ( reRoll ) {
					random = Math::RandomFloat( 0, Config::Get<int>( Vars.AntiaimDistortionRange ) );
					reRoll = false;
				}

				cur = Math::Interpolate( cur, random, Config::Get<int>( Vars.AntiaimDistortionSpeed ) / 100.f );

				cmd.viewAngles.y += cur;

				if ( std::abs( cur - random ) < 5.f )
					reRoll = true;
			}
		}
	}
}

float CAntiAim::BaseYaw( CUserCmd& cmd ) {
	m_bAntiBackstab = false;
	//if ( !Interfaces::ClientState->nChokedCommands )
	ChokeCycleJitter = !ChokeCycleJitter;

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

	if ( m_bAntiBackstab )
		return yaw;

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
					m_bAntiBackstab = true;
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

bool CAntiAim::Condition( CUserCmd& cmd, bool checkCmd ) {
	if ( !Config::Get<bool>( Vars.AntiaimEnable ) )
		return true;

	if ( ctx.m_pLocal->m_MoveType( ) == MOVETYPE_NOCLIP || ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER )
		return true;

	if ( !ctx.m_pWeapon )
		return true;

	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) )
		return true;

	if ( ctx.m_pWeapon->IsGrenade( ) && ctx.m_pWeapon->m_fThrowTime( ) )
		return true;

	if ( !checkCmd )
		return false;

	if ( cmd.iButtons & IN_ATTACK && !ctx.m_pWeapon->IsGrenade( )
		&& ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER || ctx.m_bRevolverCanShoot ) )
		return true;

	// e
	if ( cmd.iButtons & IN_USE )
		return true;

	// right click
	if ( cmd.iButtons & IN_ATTACK2 && ctx.m_pWeapon->IsKnife( ) /*&& ctx.can_shoot*/ )
		return true;

	return false;
}

void CAntiAim::FakeLag( int cmdNum ) {
	if ( Interfaces::GameRules && Interfaces::GameRules->IsFreezeTime( ) ) {
		ctx.m_bSendPacket = true;
		return;
	}

	static int maxChoke = Config::Get<int>( Vars.AntiaimFakeLagLimit );

	const int& max = Config::Get<int>( Vars.AntiaimFakeLagLimit );

	if ( !Interfaces::ClientState->nChokedCommands )
		maxChoke = Math::RandomInt( static_cast< int >( max * ( 1.f - ( static_cast< float >( Config::Get<int>( Vars.AntiaimFakeLagVariance ) ) / 100.f ) ) ), max );

	if ( ctx.m_pLocal->m_vecVelocity( ).Length( ) < 1.f
		|| Interfaces::Engine->IsVoiceRecording( ) ) {
		ctx.m_bSendPacket = Interfaces::ClientState->nChokedCommands > 1;
		return;
	}

	const auto& localData = ctx.m_cLocalData.at( Interfaces::ClientState->iLastOutgoingCommand % 150 );

	if ( Interfaces::ClientState->nChokedCommands >= maxChoke )
		ctx.m_bSendPacket = true;

	if ( Config::Get<bool>( Vars.AntiaimFakeLagBreakLC )
		&& ( ctx.m_pLocal->m_vecOrigin( ) - localData.PredictedNetvars.m_vecOrigin ).LengthSqr( ) > 4096.f )
		ctx.m_bSendPacket = true;

	if ( Config::Get<bool>( Vars.AntiaimFakeLagInPeek ) ) {
		if ( ctx.m_bInPeek )
			ctx.m_bSendPacket = true;
		else if ( cmdNum - ctx.m_iLastPeekCmdNum < 15 )
			ctx.m_bSendPacket = false;
	}
}

void CAntiAim::RunLocalModifications( CUserCmd& cmd, bool& sendPacket ) {
	const auto animstate{ ctx.m_pLocal->m_pAnimState( ) };

	if ( !Condition( cmd, true ) )
		Yaw( cmd, sendPacket );

	Features::Misc.NormalizeMovement( cmd );
	Features::Misc.MoveMINTFix( cmd, ctx.m_angOriginalViewangles, ctx.m_pLocal->m_fFlags( ), ctx.m_pLocal->m_MoveType( ) );
	Features::AnimSys.UpdateLocal( cmd.viewAngles, true, cmd );
	std::memcpy( ctx.m_pAnimationLayers, ctx.m_pLocal->m_AnimationLayers( ), 13 * sizeof CAnimationLayer );

	// we landed.
	if ( animstate->bOnGround ) {
		// walking, delay lby update by .22.
		if ( animstate->flVelocityLength2D > 0.1f )
			Features::Antiaim.m_flLowerBodyRealignTimer = ctx.m_flFixedCurtime + .22;
		// standing update every 1.1s
		else if ( ctx.m_flFixedCurtime > Features::Antiaim.m_flLowerBodyRealignTimer )
			Features::Antiaim.m_flLowerBodyRealignTimer = ctx.m_flFixedCurtime + 1.1f;
	}

	if ( !sendPacket )
		return;

	ctx.m_pLocal->SetAbsAngles( { 0.f, animstate->flAbsYaw, 0.f } );

	static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Offsets::Sigs.LookupBone ) };
	const auto boneIndex{ lookupBone( ctx.m_pLocal, _( "lean_root" ) ) };

	if ( !( ctx.m_pLocal->m_pStudioHdr( )->vecBoneFlags[ boneIndex ] & BONE_USED_BY_SERVER ) )
		ctx.m_pLocal->m_pStudioHdr( )->vecBoneFlags[ boneIndex ] = BONE_USED_BY_SERVER;

	ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight *= 2.f;
	Features::AnimSys.SetupBonesRebuilt( ctx.m_pLocal, ctx.m_matRealLocalBones, BONE_USED_BY_SERVER,
		Interfaces::Globals->flCurTime, true );
	std::memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_matRealLocalBones, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4a_t ) );

	ctx.m_vecSetupBonesOrigin = ctx.m_pLocal->GetAbsOrigin( );
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