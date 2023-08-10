#include "ragebot.h"

bool CRageBot::ExtrapolatePlayer( PlayerEntry& entry, float yaw, int resolverSide, int amount, Vector previousVelocity ) {
	if ( yaw == -1 )
		yaw = entry.m_flPreviousYaws.back( ).m_flYaw;

	PreviousExtrapolationData_t previous{ entry.m_pPlayer->m_flDuckAmount( ), entry.m_pPlayer->m_fFlags( ) };
	auto angles{ entry.m_pRecords.back( )->m_angEyeAngles };
	angles.y = yaw;

	float assumedSpeed{ };

	const auto dir{ std::remainder( RAD2DEG(
		std::atan2( entry.m_optPreviousData->m_vecVelocity.y, entry.m_optPreviousData->m_vecVelocity.x ) ), 360.f ) };

	const auto prevDir{ std::remainder( RAD2DEG(
		std::atan2( previousVelocity.y, previousVelocity.x ) ), 360.f ) };

	auto direction{ std::remainder( RAD2DEG(
	std::atan2( ( entry.m_pPlayer->m_vecVelocity( ) - previousVelocity ).y, ( entry.m_pPlayer->m_vecVelocity( ) - previousVelocity ).x ) ), 360.f ) };


	if ( std::abs( dir - prevDir ) >= 5.f ) {

	}
	else {
		if ( std::abs( entry.m_optPreviousData->m_vecVelocity.Length2D( ) - previousVelocity.Length2D( ) ) < 10.f )
			assumedSpeed = entry.m_optPreviousData->m_vecVelocity.Length2D( );

		direction = dir;
	}

	for ( int i{ 1 }; i <= amount; ++i ) {
		SimulatePlayer( entry.m_pPlayer, entry.m_pPlayer->m_flSimulationTime( ) + TICKS_TO_TIME( i ),
			angles, 0, i == amount, previous, 
			direction, assumedSpeed, false );

		previous = { entry.m_pPlayer->m_flDuckAmount( ), entry.m_pPlayer->m_fFlags( ) };
	}

	return true;
}

bool CRageBot::ExtrapolatePlayer( CBasePlayer* player, float baseTime, int amount, QAngle angles, Vector previousVelocity, bool local ) {
	const auto backupJumping{ Features::AnimSys.m_bJumping };
	const auto backupLowerBodyRealignTimer{ Features::AnimSys.m_flLowerBodyRealignTimer };

	PreviousExtrapolationData_t previous{ player->m_flDuckAmount( ), player->m_fFlags( ) };

	float assumedSpeed{ };

	const auto dir = std::remainder( RAD2DEG(
		atan2( player->m_vecVelocity( ).y, player->m_vecVelocity( ).x ) ), 360.f );
	const auto prev_dir = std::remainder( RAD2DEG(
		atan2( previousVelocity.y, previousVelocity.x ) ), 360.f );

	if ( abs( dir - prev_dir ) >= 5.f
		&& !local )
		return false;

	if ( std::abs( player->m_vecVelocity( ).Length2D( ) - previousVelocity.Length2D( ) ) < 10.f )
		assumedSpeed = player->m_vecVelocity( ).Length2D( );

	const auto direction = dir;

	for ( int i{ 1 }; i <= amount; ++i ) {
		SimulatePlayer( player, baseTime + TICKS_TO_TIME( i ),
			angles, 0, i == amount, previous, 
			direction, assumedSpeed, local );

		previous = { player->m_flDuckAmount( ), player->m_fFlags( ) };
	}

	if ( local ) {
		Features::AnimSys.m_bJumping = backupJumping;
		Features::AnimSys.m_flLowerBodyRealignTimer = backupLowerBodyRealignTimer;
	}

	return true;
}

void CRageBot::SimulatePlayer( CBasePlayer* player, float time, QAngle angles, int resolverSide, bool last, PreviousExtrapolationData_t& previous, float direction, float assumedSpeed, bool local ) {
	CUserCmd cmd;
	CMoveData moveData;
	memset( &moveData, 0, sizeof( CMoveData ) );
	memset( &cmd, 0, sizeof( CUserCmd ) );
	cmd.viewAngles = angles;
	cmd.iTickCount = TICKS_TO_TIME( time );
	cmd.iCommandNumber = Interfaces::ClientState->nChokedCommands + Interfaces::ClientState->iLastOutgoingCommand + 1;
	const auto& curFlags{ player->m_fFlags( ) };

	const auto backupAttack{ player->m_flNextAttack( ) };

	if ( curFlags & FL_ONGROUND && !( previous.m_iFlags & FL_ONGROUND ) ) {
		if ( !local )
			player->m_flNextAttack( ) = time;

		cmd.iButtons |= IN_JUMP;
	}
	else if ( !local )
		player->m_flNextAttack( ) = 0.f;

	if ( ( player->m_flDuckAmount( ) > 0.0f && player->m_flDuckAmount( ) >= previous.m_flDuckAmount )
		|| curFlags & FL_DUCKING || curFlags & FL_ANIMDUCKING )
		cmd.iButtons |= IN_DUCK;

	// walking
	if ( player->m_iMoveState( ) == 1 )
		cmd.iButtons |= IN_SPEED;


	// not accelerating/decelerating- assume same pace
	if ( assumedSpeed )
		cmd.flForwardMove = assumedSpeed;
	else
		cmd.flForwardMove = 450.f;

	// assume they will stop
	// TODO: more logic?
	if ( !local && player->m_fFlags( ) & FL_ONGROUND ) {
		const auto hitboxSet{ ctx.m_pLocal->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( ctx.m_pLocal->m_nHitboxSet( ) ) };

		auto enemyShootPos{ player->m_vecOrigin( ) };
		enemyShootPos.z += ( player->m_fFlags( ) & FL_DUCKING ) ? 46.f : 64.f;

		const auto hitbox{ hitboxSet->GetHitbox( HITBOX_CHEST ) };
		const auto center{ Math::VectorTransform( ( hitbox->vecBBMax + hitbox->vecBBMin ) * 0.5f,
					ctx.m_pLocal->m_CachedBoneData( ).Base( )[ hitbox->iBone ] ) };

		const auto& weapon{ player->GetWeapon( ) };
		if ( !weapon )
			return;

		const auto data{ Features::Autowall.FireBullet( player, ctx.m_pLocal, weapon->GetCSWeaponData( ),
			weapon->m_iItemDefinitionIndex( ) == WEAPON_TASER, enemyShootPos, center, true ) };

		if ( data.dmg > 0 ) {
			const auto maxWeaponSpeed{ player->m_flMaxSpeed( ) };
			auto optSpeed{ maxWeaponSpeed / 3.f };

			Features::Misc.LimitSpeed( cmd, optSpeed, player );
		}
	}

	auto moveDir{ cmd.viewAngles };

	moveDir.y = direction;
	Features::Misc.MoveMINTFix( cmd, moveDir, player->m_fFlags( ), player->m_MoveType( ) );

	if ( !last ) {
		if ( resolverSide == 1 )
			cmd.viewAngles.y += 120.f;
		else if ( resolverSide == 2 )
			cmd.viewAngles.y -= 120.f;

		cmd.viewAngles.Normalize( );
	}

	const auto originalplayercommand{ !player->CurrentCommand( ) ? CUserCmd( ) : *player->CurrentCommand( ) };
	//const auto pPredictionPlayer{ *( *reinterpret_cast< CBasePlayer*** >( Displacement::Sigs.pPredictionPlayer ) ) };
	//const auto originalrandomseed{ *( *reinterpret_cast< unsigned int** >( Displacement::Sigs.uPredictionRandomSeed ) ) };

	const auto backupMaxsZ{ player->m_vecMaxs( ).z };
	const auto backupDucking{ ( player->m_fFlags( ) & FL_DUCKING ) };
	const auto backupTickbase{ player->m_nTickBase( ) };

	const auto backupInPrediction{ Interfaces::Prediction->bInPrediction };
	const auto backupFrametime{ Interfaces::Globals->flFrameTime };
	const auto backupCurtime{ Interfaces::Globals->flCurTime };

	Interfaces::Prediction->bInPrediction = true;
	Interfaces::MoveHelper->SetHost( player );

	if ( player->CurrentCommand( ) )
		player->CurrentCommand( ) = &cmd;

	// TODO:
	//*( *reinterpret_cast< unsigned int** >( Displacement::Sigs.uPredictionRandomSeed ) ) = ( ( int( __thiscall* )( int ) )Displacement::Sigs.MD5PseudoRandom )( cmd.iCommandNumber ) & 0x7fffffff;
	//*( *reinterpret_cast< CBasePlayer*** >( Displacement::Sigs.pPredictionPlayer ) ) = player;

	Interfaces::Globals->flFrameTime = Interfaces::Prediction->bEnginePaused ? 0.0f : Interfaces::Globals->flIntervalPerTick;
	Interfaces::Globals->flCurTime = time;

	ctx.m_bProhibitSounds = true;
	Interfaces::Prediction->CheckMovingGround( player, Interfaces::Globals->flFrameTime );
	Interfaces::Prediction->SetupMove( player, &cmd, Interfaces::MoveHelper, &moveData );
	Interfaces::GameMovement->ProcessMovement( player, &moveData );
	Interfaces::Prediction->FinishMove( player, &cmd, &moveData );
	Interfaces::MoveHelper->SetHost( nullptr );
	ctx.m_bProhibitSounds = false;

	/*if ( player->m_fFlags( ) & FL_DUCKING ) {
		player->SetCollisionBounds( { -16.f, -16.f, 0.f }, { 16.f, 16.f, 54.f } );
		player->m_vecViewOffset( ).z = 46.f;
	}
	else {
		player->SetCollisionBounds( { -16.f, -16.f, 0.f }, { 16.f, 16.f, 72.f } );
		player->m_vecViewOffset( ).z = 64.f;
	}

	if ( ( player->m_fFlags( ) & FL_DUCKING ) != backupDucking ) {
		// rebuild: server.dll/client.dll @ 55 8B EC 8B 45 10 F3 0F 10 81
		player->m_flNewBoundsMaxs( ) = player->m_flUnknownVar( ) + backupMaxsZ;
		player->m_flNewBoundsTime( ) = time;
	}*/

	player->m_vecAbsVelocity( ) = player->m_vecVelocity( );
	player->SetAbsOrigin( player->m_vecOrigin( ) );
	player->m_iEFlags( ) &= ~EFL_DIRTY_ABSVELOCITY;

	Interfaces::Prediction->bInPrediction = backupInPrediction;

	// animation
	if ( !local ) {
		const auto state{ player->m_pAnimState( ) };

		if ( state->iLastUpdateFrame == Interfaces::Globals->iFrameCount )
			state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

		player->m_angEyeAngles( ) = cmd.viewAngles;

		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
		player->UpdateClientsideAnimations( );
		player->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;
	}
	else {
		player->m_nTickBase( ) = TIME_TO_TICKS( Interfaces::Globals->flCurTime );
		Features::AnimSys.UpdateLocal( cmd.viewAngles, false, cmd );
	}

	if ( player->CurrentCommand( ) )
		*player->CurrentCommand( ) = originalplayercommand;
	//*( *reinterpret_cast< unsigned int** >( Displacement::Sigs.uPredictionRandomSeed ) ) = originalrandomseed;
	//*( *reinterpret_cast< CBasePlayer*** >( Displacement::Sigs.pPredictionPlayer ) ) = pPredictionPlayer;

	player->m_flNextAttack( ) = backupAttack;
	player->m_nTickBase( ) = backupTickbase;

	Interfaces::Globals->flCurTime = backupCurtime;
	Interfaces::Globals->flFrameTime = backupFrametime;
}