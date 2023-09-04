#include "antiaim.h"
#include "../animations/animation.h"
#include "../rage/exploits.h"
// TODO: antiresolver!
// check if we are able to be resolved from animlayers, and balance adjust then just flip desync
// also force balance adjust to be triggered when stopping or randomly when we are standing

float CAntiAim::Pitch( ) {
	const auto cond{ ctx.m_bSafeFromDefensive && Features::Exploits.m_bWasDefensiveTick ? Config::Get<int>( Vars.AntiaimSafePitch ) : Config::Get<int>( Vars.AntiaimPitch ) };

	switch ( cond ) {
	case 1: return -89.f;// up
	case 2: return  89.f;// down
	case 3: return  0.f ;// zero
	case 4: return Math::RandomFloat( -89.f, 89 );// random
	default: break;
	}
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

	if ( ctx.m_bSafeFromDefensive && Features::Exploits.m_bWasDefensiveTick ) {
		const auto amount{ Config::Get<int>( Vars.AntiaimSafeYawRandomisation ) / 100.f };
		yaw += Math::RandomFloat( -180 * amount, 180 * amount );
	}
}

int CAntiAim::Freestanding( ) {
	if ( !Config::Get<int>( Vars.AntiaimFreestanding )
		|| !Config::Get<keybind_t>( Vars.AntiaimFreestandingKey ).enabled )
		return 0;

	Vector forward, right;
	Math::AngleVectors( ctx.m_angOriginalViewangles, &forward, &right );

	CGameTrace tr;

	// middle
	Interfaces::EngineTrace->TraceRay(
		{ ctx.m_vecEyePos, ctx.m_vecEyePos + forward * 100.0f }, MASK_PLAYERSOLID,
		nullptr, &tr
	);
	const auto middleDist{ ( tr.vecEnd - tr.vecStart ).Length( ) };

	// right
	Interfaces::EngineTrace->TraceRay(
		{ ctx.m_vecEyePos + right * 35.0f, ( ctx.m_vecEyePos + forward * 100.0f ) + right * 35.0f }, MASK_PLAYERSOLID,
		nullptr, &tr
	);
	const auto rightDist{  ( tr.vecEnd - tr.vecStart ).Length( ) };

	// left
	Interfaces::EngineTrace->TraceRay(
		{ ctx.m_vecEyePos - right * 35.0f, ( ctx.m_vecEyePos + forward * 100.0f ) - right * 35.0f }, MASK_PLAYERSOLID,
		nullptr, &tr
	);
	const auto leftDist{ ( tr.vecEnd - tr.vecStart ).Length( ) };

	if ( rightDist > leftDist + 20.f ) {
		if ( rightDist > middleDist + 20.f )
			return 1;
	}

	if ( leftDist > rightDist + 20.f ) {
		if ( leftDist > middleDist + 20.f )
			return 2;
	}

	return 0;
}

float CAntiAim::BaseYaw( CUserCmd& cmd ) {
	m_bAntiBackstab = false;
	//if ( !Interfaces::ClientState->nChokedCommands )
	ChokeCycleJitter = !ChokeCycleJitter;

	static auto old{ Config::Get<keybind_t>( Vars.AntiaimInvert ).enabled };
	if ( old != Config::Get<keybind_t>( Vars.AntiaimInvert ).enabled )
		Invert = old = Config::Get<keybind_t>( Vars.AntiaimInvert ).enabled;

	auto yaw = cmd.viewAngles.y;

	const auto side{ Freestanding( ) };

	if ( ManualSide )
		m_iChoiceSide = ManualSide;
	else if ( Config::Get<int>( Vars.AntiaimFreestanding ) == 1 )
		m_iChoiceSide = side;

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

	if ( side && Config::Get<int>( Vars.AntiaimFreestanding ) == 1 ) {
		if ( side == 1 ) {
			yaw += 90.f;
			return yaw;
		}
		else if ( side == 2 ) {
			yaw -= 90.f;
			return yaw;
		}
	}
	else if ( Config::Get<int>( Vars.AntiaimFreestanding ) == 2 || Config::Get<int>( Vars.AntiaimFreestanding ) == 3 ) {
		if ( side == 1 )
			Invert = Config::Get<int>( Vars.AntiaimFreestanding ) == 2;
		else if ( side == 2 )
			Invert = Config::Get<int>( Vars.AntiaimFreestanding ) != 2;
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

	for ( auto i = 1; i < 64; ++i ) {
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

	if ( Config::Get<bool>( Vars.AntiaimDesync ) )
		maxChoke = std::max( maxChoke, 1 );

	if ( Interfaces::Engine->IsVoiceRecording( ) )
		maxChoke = 1;

	if ( ctx.m_pLocal->m_vecVelocity( ).Length2D( ) < 0.1f )
		maxChoke = 1;

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled )
		maxChoke = 15;

	const auto& localData = ctx.m_cLocalData.at( Interfaces::ClientState->iLastOutgoingCommand % 150 );

	if ( Interfaces::ClientState->nChokedCommands >= maxChoke )
		ctx.m_bSendPacket = true;

	if ( Config::Get<bool>( Vars.AntiaimFakeLagBreakLC )
		&& ( ctx.m_pLocal->m_vecOrigin( ) - localData.PredictedNetvars.m_vecOrigin ).LengthSqr( ) > 4096.f )
		ctx.m_bSendPacket = true;

	if ( Config::Get<bool>( Vars.AntiaimFakeLagInPeek ) ) {
		
		if ( cmdNum - ctx.m_iLastPeekCmdNum < 15 ) {
			ctx.m_bSendPacket = false;
			//return;
		}
		else if ( ctx.m_bInPeek )
			ctx.m_bSendPacket = true;
	}
}

void CAntiAim::RunLocalModifications( CUserCmd& cmd, int tickbase ) {
	const auto pitch{ Pitch( ) };

	const auto animstate{ ctx.m_pLocal->m_pAnimState( ) };
	const auto totalCmds{ Interfaces::ClientState->nChokedCommands + 1 };
	if ( totalCmds < 1
		|| !animstate )
		return;

	float yaw{ std::remainderf( BaseYaw( cmd ), 360.f ) };

	if ( ctx.m_bSafeFromDefensive && Features::Exploits.m_bWasDefensiveTick ) {
		const auto amount{ Config::Get<int>( Vars.AntiaimSafeYawRandomisation ) / 100.f };
		yaw += Math::RandomFloat( -180 * amount, 180 * amount );
	}

	const auto m_flVelocityLengthXY{ ctx.m_pLocal->m_vecVelocity( ).Length2D( ) };

	if ( m_flVelocityLengthXY <= 0.1f ) {
		if ( Config::Get<bool>( Vars.AntiaimDistortion ) ) {
			static bool reRoll{ };
			static float random{ };
			static float cur{ };

			if ( Config::Get<bool>( Vars.AntiaimDistortionSpike ) ) {
				random = Math::RandomFloat( 0, Config::Get<int>( Vars.AntiaimDistortionRange ) );
				yaw += random;
			}
			else {
				if ( reRoll ) {
					random = Math::RandomFloat( 0, Config::Get<int>( Vars.AntiaimDistortionRange ) );
					reRoll = false;
				}

				cur = Math::Interpolate( cur, random, Config::Get<int>( Vars.AntiaimDistortionSpeed ) / 100.f );

				yaw += cur;

				if ( std::abs( cur - random ) < 5.f )
					reRoll = true;
			}
		}
	}

	const auto inShot{ ctx.m_iLastShotNumber > Interfaces::ClientState->iLastOutgoingCommand
		&& ctx.m_iLastShotNumber <= ( Interfaces::ClientState->iLastOutgoingCommand + Interfaces::ClientState->nChokedCommands + 1 ) };

	if ( animstate->bFirstUpdate )
		ctx.m_cFakeData.init = false;

	const auto backupState{ *animstate };
	CAnimationLayer backupLayers[ 13 ]{ };
	std::memcpy( backupLayers, ctx.m_pLocal->m_AnimationLayers( ), 13 * sizeof CAnimationLayer );

	bool did{ };

	for ( auto i{ 1 }; i <= totalCmds; ++i ) {
		const auto j{ ( Interfaces::ClientState->iLastOutgoingCommand + i ) % 150 };

		auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
		auto& curLocalData{ ctx.m_cLocalData.at( j ) };

		const auto lastCmd{ i == totalCmds };

		if ( curLocalData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) )
			continue;

		if ( curUserCmd.iTickCount == INT_MAX )
			continue;

		const auto oldViewAngles{ curUserCmd.viewAngles };

		if ( i == 1 ) {
			ctx.m_pLocal->m_nTickBase( ) = tickbase - ( totalCmds - i );
			ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
			ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
			ctx.m_pLocal->m_vecVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
			ctx.m_pLocal->m_flDuckAmount( ) = curLocalData.PredictedNetvars.m_flDuckAmount;

			if ( curLocalData.PredictedNetvars.m_MoveType != MOVETYPE_LADDER
				&& curLocalData.m_MoveType != MOVETYPE_LADDER ) {
				if ( curLocalData.m_bCanAA ) {

					curUserCmd.viewAngles.y = yaw;
					curUserCmd.viewAngles.x = pitch;// not needed just using it for local anims

					if ( Config::Get<bool>( Vars.AntiaimDesync ) ) {
						if ( curLocalData.PredictedNetvars.m_vecVelocity.Length2D( ) < 0.1f ) {

							if ( TICKS_TO_TIME( tickbase - ( totalCmds - i ) ) > Features::AnimSys.m_flLowerBodyRealignTimer ) {
								ctx.m_bFlicking = true;

								if ( Config::Get<bool>( Vars.AntiaimStaticBreak ) ) {
									curUserCmd.viewAngles.y = ctx.m_angOriginalViewangles.y;
									if ( m_iChoiceSide == 1 )
										curUserCmd.viewAngles.y += 90.f;
									else if ( m_iChoiceSide == 2 )
										curUserCmd.viewAngles.y -= 90.f;
								}

								curUserCmd.viewAngles.y += Config::Get<int>( Vars.AntiaimBreakLBYAngle );
								if ( std::abs( Math::AngleDiff( ctx.m_pLocal->m_pAnimState( )->flAbsYaw, ctx.m_angOriginalViewangles.y ) ) <= 35.f )
									curUserCmd.viewAngles.y = ctx.m_pLocal->m_pAnimState( )->flAbsYaw + 40.f;
							}
						}
					}


					//Features::Misc.NormalizeMovement( curUserCmd );

					Features::Misc.MoveMINTFix(
						curUserCmd, oldViewAngles,
						curLocalData.PredictedNetvars.m_iFlags,
						curLocalData.PredictedNetvars.m_MoveType
					);
				}

				Features::Misc.NormalizeMovement( curUserCmd );
			}

			if ( ctx.m_pLocal->m_MoveType( ) == MOVETYPE_WALK ) {
				curUserCmd.iButtons &= ~( IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT );

				if ( curUserCmd.flForwardMove != 0.f )
					curUserCmd.iButtons |=
					( Config::Get<bool>( Vars.MiscSlideWalk ) ? curUserCmd.flForwardMove < 0.f : curUserCmd.flForwardMove > 0.f )
					? IN_FORWARD : IN_BACK;

				if ( curUserCmd.flSideMove ) {
					curUserCmd.iButtons |=
						( Config::Get<bool>( Vars.MiscSlideWalk ) ? curUserCmd.flSideMove < 0.f : curUserCmd.flSideMove > 0.f )
						? IN_MOVERIGHT : IN_MOVELEFT;
				}
			}

			ctx.m_pLocal->m_nTickBase( ) = tickbase - ( totalCmds - i );
			ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
			ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
			ctx.m_pLocal->m_flDuckAmount( ) = curLocalData.PredictedNetvars.m_flDuckAmount;

			Features::AnimSys.UpdateLocal( curUserCmd.viewAngles, lastCmd, curUserCmd );
		}
		

		if ( lastCmd ) {
			if ( Config::Get<bool>( Vars.AntiaimStaticNetwork ) ) {
				curUserCmd.viewAngles.y = cmd.viewAngles.y;
				if ( m_iChoiceSide == 1 )
					curUserCmd.viewAngles.y += 90.f;
				else if ( m_iChoiceSide == 2 )
					curUserCmd.viewAngles.y -= 90.f;
			}
			else
				curUserCmd.viewAngles.y = yaw;

			curUserCmd.viewAngles.x = pitch;
			curUserCmd.viewAngles.y += Config::Get<int>( Vars.AntiaimNetworkedAngle );

			Features::Misc.MoveMINTFix(
				curUserCmd, oldViewAngles,
				curLocalData.PredictedNetvars.m_iFlags,
				curLocalData.PredictedNetvars.m_MoveType
			);

			Features::Misc.NormalizeMovement( curUserCmd );
		}

		Interfaces::Input->pVerifiedCommands[ j ].userCmd = curUserCmd;
		Interfaces::Input->pVerifiedCommands[ j ].uHashCRC = curUserCmd.GetChecksum( );
	}

	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight = ctx.m_pAnimationLayers[ 3 ].flWeight;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle = ctx.m_pAnimationLayers[ 3 ].flCycle;
	std::memcpy( ctx.m_pAnimationLayers, ctx.m_pLocal->m_AnimationLayers( ), 13 * sizeof CAnimationLayer );

	/*if ( ( Features::Exploits.m_iShiftAmount
		&& !Features::Exploits.m_bRealCmds )
		|| ( ctx.m_bSafeFromDefensive && Features::Exploits.m_bWasDefensiveTick ) )
		return;*/

	ctx.m_pLocal->SetAbsAngles( { 0.f, animstate->flAbsYaw, 0.f } );

	static auto lookupBone{ *reinterpret_cast< int( __thiscall* )( void*, const char* ) >( Displacement::Sigs.LookupBone ) };
	const auto boneIndex{ lookupBone( ctx.m_pLocal, _( "lean_root" ) ) };

	if ( ctx.m_pLocal->m_pStudioHdr( )->vecBoneFlags[ boneIndex ] != BONE_USED_BY_SERVER )
		ctx.m_pLocal->m_pStudioHdr( )->vecBoneFlags[ boneIndex ] = BONE_USED_BY_SERVER;

	const auto backup12Weight{ ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight };
	const auto backup3Weight{ ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight };
	const auto backup3Cycle{ ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle };

	ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight *= 2.f;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle = 0.f;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight = 0.f;

	Features::AnimSys.SetupBonesRebuilt( ctx.m_pLocal, ctx.m_matRealLocalBones, BONE_USED_BY_SERVER,
		Interfaces::Globals->flCurTime, true );
	std::memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_matRealLocalBones, ctx.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4a_t ) );

	ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight = backup12Weight;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle = backup3Cycle;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight = backup3Weight;

	ctx.m_vecSetupBonesOrigin = ctx.m_pLocal->GetAbsOrigin( );

	if ( !Config::Get<bool>( Vars.ChamDesync ) )
		return;

	const auto backupState2{ *animstate };

	if ( ctx.m_cFakeData.init )
		*animstate = ctx.m_cFakeData.m_sState;
	else
		*animstate = backupState;

	std::memcpy( ctx.m_pLocal->m_AnimationLayers( ), backupLayers, 13 * sizeof CAnimationLayer );

	const auto j{ ( Interfaces::ClientState->iLastOutgoingCommand + 1 ) % 150 };

	auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
	auto& curLocalData{ ctx.m_cLocalData.at( j ) };

	ctx.m_pLocal->m_nTickBase( ) = tickbase - totalCmds + 1;
	ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
	ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;

	Features::AnimSys.UpdateLocal( Interfaces::Input->pCommands[ ( Interfaces::ClientState->iLastOutgoingCommand + totalCmds ) % 150 ].viewAngles, true, curUserCmd );

	ctx.m_pLocal->SetAbsAngles( { 0.f, animstate->flAbsYaw, 0.f } );

	// enemy gets the real layers networked to them
	std::memcpy( ctx.m_pLocal->m_AnimationLayers( ), ctx.m_pAnimationLayers, 13 * sizeof CAnimationLayer );

	const auto backupWeight{ ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight };
	ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight *= 2.f;
	Features::AnimSys.SetupBonesRebuilt( ctx.m_pLocal, ctx.m_cFakeData.m_matMatrix, BONE_USED_BY_SERVER | BONE_USED_BY_BONE_MERGE,
		Interfaces::Globals->flCurTime, true );
	ctx.m_pLocal->m_AnimationLayers( )[ 12 ].flWeight = backupWeight;

	ctx.m_cFakeData.m_sState = *animstate;
	ctx.m_cFakeData.init = true;
	*animstate = backupState2;

	std::memcpy( ctx.m_pLocal->m_CachedBoneData( ).Base( ), ctx.m_matRealLocalBones, ctx.m_pLocal->m_CachedBoneData( ).Size( ) * sizeof( matrix3x4_t ) );
}

// pasta reis courtesy of slazy
/*
bool CAntiAim::AutoDirection( float& yaw ) {
	if ( !Config::Get<int>( Vars.AntiaimFreestand ) )
		return false;

	CBasePlayer* best_player{ };
	auto best_fov = std::numeric_limits< float >::max( );

	const auto view_angles = ctx.m_angOriginalViewangles;

	for ( auto i = 1; i < 64; ++i ) {
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