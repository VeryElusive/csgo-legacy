#include "animation.h"

void CAnimationSys::UpdateLocalFull( CUserCmd& cmd, bool sendPacket ) {
	auto i = 1;
	auto chokedCmds = Interfaces::ClientState->nChokedCommands;

	const auto totalCmds = chokedCmds + 1;
	if ( totalCmds < 1
		|| !sendPacket )
		return;

	if ( ctx.m_bFilledAnims ) {
		ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
		ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
		*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;
	}

	const auto inShot = ctx.m_iLastShotNumber > Interfaces::ClientState->iLastOutgoingCommand
		&& ctx.m_iLastShotNumber <= ( Interfaces::ClientState->iLastOutgoingCommand + Interfaces::ClientState->nChokedCommands + 1 );

	const auto condition{ Features::Antiaim.Condition( cmd ) };

	for ( ; i <= totalCmds; ++i, --chokedCmds ) {
		const auto j = ( Interfaces::ClientState->iLastOutgoingCommand + i ) % 150;

		auto& curUserCmd{ Interfaces::Input->pCommands[ j ] };
		auto& curLocalData{ ctx.m_cLocalData.at( j ) };

		if ( curLocalData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) )
			continue;

		if ( curUserCmd.iTickCount >= INT_MAX )
			continue;

		if ( !condition 
			&& ctx.m_pLocal->m_vecVelocity( ).Length2D( ) > 1.f
			&& curLocalData.PredictedNetvars.m_MoveType != MOVETYPE_LADDER
			&& curLocalData.m_MoveType != MOVETYPE_LADDER ) {

			if ( !curLocalData.m_bThrowingNade
				&& !( curUserCmd.iButtons & IN_ATTACK && ( !ctx.m_pWeapon->IsGrenade( ) || ctx.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER || !curLocalData.m_bRevolverCock ) )
				&& !( curUserCmd.iButtons & IN_ATTACK2 && ctx.m_pWeapon->IsKnife( ) )
				&& !( curUserCmd.iButtons & IN_USE ) ) {
				const auto oldViewAngles{ curUserCmd.viewAngles };

				if ( chokedCmds > 0 && !inShot && Config::Get<bool>( Vars.AntiaimDesync ) )
					curUserCmd.viewAngles.y = std::remainderf( curUserCmd.viewAngles.y + ( Features::Antiaim.ChokeCycleJitter ? -120.f : 120.f ), 360.f );

				Features::Misc.MoveMINTFix(
					curUserCmd, oldViewAngles,
					curLocalData.PredictedNetvars.m_iFlags,
					curLocalData.PredictedNetvars.m_MoveType
				);
			}

			Features::Misc.NormalizeMovement( curUserCmd );
		}

		ctx.m_pLocal->m_nTickBase( ) = curLocalData.PredictedNetvars.m_nTickBase;
		ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
		ctx.m_pLocal->m_vecVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
		ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecAbsVelocity;

		const auto lastCmd{ curUserCmd.iCommandNumber == cmd.iCommandNumber };
		Features::AnimSys.UpdateLocal( curUserCmd.viewAngles, lastCmd );
	}
	const auto state{ ctx.m_pLocal->m_pAnimState( ) };

	// we landed.
	if ( state->bOnGround ) {
		// walking, delay lby update by .22.
		if ( state->flVelocityLength2D > 0.1f )
			Features::Antiaim.m_flLowerBodyRealignTimer = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) );
		// standing update every 1.1s
		else if ( Config::Get<bool>( Vars.RagebotLagcompensation )
			&& TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) ) > Features::Antiaim.m_flLowerBodyRealignTimer )
			Features::Antiaim.m_flLowerBodyRealignTimer = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) ) + 1.1f;
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

void CAnimationSys::UpdateLocal( const QAngle& view_angles, const bool only_anim_state ) {
	const auto anim_state = ctx.m_pLocal->m_pAnimState( );
	if ( !anim_state )
		return;

	const auto backup_cur_time = Interfaces::Globals->flCurTime;
	const auto backup_frame_time = Interfaces::Globals->flFrameTime;

	Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) );
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;

	ctx.m_pLocal->m_vecRenderAngles( ) = view_angles;

	anim_state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

	const auto backup_abs_velocity = ctx.m_pLocal->m_vecAbsVelocity( );

	ctx.m_pLocal->m_vecAbsVelocity( ) = ctx.m_pLocal->m_vecVelocity( );

	const auto backup_eflags = ctx.m_pLocal->m_iEFlags( );

	ctx.m_pLocal->m_iEFlags( ) &= ~EFL_DIRTY_ABSVELOCITY;

	const auto backup_client_side_anim = ctx.m_pLocal->m_bClientSideAnimation( );

	ctx.m_pLocal->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;

	for ( int i = 1; i <= ctx.m_pLocal->m_iAnimationLayersCount( ); i++ )
		ctx.m_pLocal->m_AnimationLayers( )[ i ].pOwner = ctx.m_pLocal;

	if ( only_anim_state )
		anim_state->Update( view_angles );
	else
		ctx.m_pLocal->UpdateClientsideAnimations( );

	ctx.m_pLocal->m_bClientSideAnimation( ) = backup_client_side_anim;

	ctx.m_bUpdatingAnimations = false;

	ctx.m_pLocal->m_iEFlags( ) = backup_eflags;

	ctx.m_pLocal->m_vecAbsVelocity( ) = backup_abs_velocity;

	Interfaces::Globals->flCurTime = backup_cur_time;
	Interfaces::Globals->flFrameTime = backup_frame_time;
}

void CAnimationSys::SetupLocalMatrix( ) {
	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	const auto state = ctx.m_pLocal->m_pAnimState( );
	if ( !state )
		return;

	const auto& localData = ctx.m_cLocalData.at( ctx.m_iLastSentCmdNumber % 150 );
	if ( localData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) )
		return;

	auto RestoreAnims = [ ]( CAnimationLayer layers[ 13 ], std::array<float, 24> pose, float absAngles, CCSGOPlayerAnimState animstate ) ->void {
		memcpy( ctx.m_pLocal->m_AnimationLayers( ), layers, 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
		ctx.m_pLocal->m_flPoseParameter( ) = pose;
		ctx.m_pLocal->SetAbsAngles( { 0, absAngles, 0 } );
		*ctx.m_pLocal->m_pAnimState( ) = animstate;
	};

	CAnimationLayer backupLayers[ 13 ];
	memcpy( backupLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
	const auto backupPose{ ctx.m_pLocal->m_flPoseParameter( ) };
	const auto backupAbsAngle{ ctx.m_pLocal->GetAbsAngles( ).y };
	const auto backupAnimstate{ *ctx.m_pLocal->m_pAnimState( ) };

	RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );

	if ( !ctx.m_bFilledAnims )
		return;

	memcpy( ctx.m_pLocal->m_AnimationLayers( ), ctx.m_pAnimationLayers, 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
	ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
	ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
	*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;


	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle = 0.f;
	ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight = 0.f;

	SetupBonesFixed( ctx.m_pLocal, ctx.m_matRealLocalBones, BONE_USED_BY_ANYTHING | BONE_ALWAYS_SETUP,
		Interfaces::Globals->flCurTime, ( INVALIDATEBONECACHE | SETUPBONESFRAME | /*NULLIK |*/ OCCLUSIONINTERP ) );

	RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );
}