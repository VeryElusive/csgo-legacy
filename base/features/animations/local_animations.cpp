#include "animation.h"

/*void CAnimationSys::UpdateCommands( ) {
	if ( ctx.m_pQueuedCommands.empty( ) )
		return;
	
	if ( ctx.m_pLocal->IsDead( ) ) {
		if ( !ctx.m_pQueuedCommands.empty( ) )
			ctx.m_pQueuedCommands.clear( );

		return;
	}

	CAnimationLayer BackupAnimLayers[ 13 ];
	memcpy( BackupAnimLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );

	const int lastCmd{ ctx.m_pQueuedCommands.back( ).second };

	const auto backupTickbase{ ctx.m_pLocal->m_nTickBase( ) };
	const auto backupFlags{ ctx.m_pLocal->m_fFlags( ) };
	const auto backupVelocity{ ctx.m_pLocal->m_vecVelocity( ) };
	const auto backupAbsVelocitys{ ctx.m_pLocal->m_vecAbsVelocity( ) };

	if ( ctx.m_bFilledAnims ) {
		ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
		ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
		*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;
	}

	for ( const auto& command : ctx.m_pQueuedCommands ) {
		auto& curLocalData{ ctx.m_cLocalData.at( command.second ) };

		ctx.m_pLocal->m_nTickBase( ) = curLocalData.PredictedNetvars.m_nTickBase;
		ctx.m_pLocal->m_fFlags( ) = curLocalData.PredictedNetvars.m_iFlags;
		ctx.m_pLocal->m_vecVelocity( ) = curLocalData.PredictedNetvars.m_vecVelocity;
		ctx.m_pLocal->m_vecAbsVelocity( ) = curLocalData.PredictedNetvars.m_vecAbsVelocity;

		Features::AnimSys.UpdateLocal( command.first, lastCmd != command.second );
	}

	memcpy( ctx.m_pAnimationLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
	ctx.m_flPoseParameter = ctx.m_pLocal->m_flPoseParameter( );
	ctx.m_cAnimstate = *ctx.m_pLocal->m_pAnimState( );
	ctx.m_flAbsYaw = ctx.m_cAnimstate.flAbsYaw;
	ctx.m_bFilledAnims = true;


	ctx.m_pLocal->m_nTickBase( ) = backupTickbase;
	ctx.m_pLocal->m_fFlags( ) = backupFlags;
	ctx.m_pLocal->m_vecVelocity( ) = backupVelocity;
	ctx.m_pLocal->m_vecAbsVelocity( ) = backupAbsVelocitys;

	ctx.m_pQueuedCommands.clear( );

	memcpy( ctx.m_pLocal->m_AnimationLayers( ), BackupAnimLayers, 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
}*/
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

	if ( Config::Get<bool>( Vars.ChamDesync ) ) {
		RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );

		if ( ctx.m_cFakeData.m_flSpawnTime == 0.f
			|| ctx.m_cFakeData.m_flSpawnTime != ctx.m_pLocal->m_flSpawnTime( ) ) {
			ctx.m_cFakeData.m_AnimState = *ctx.m_pLocal->m_pAnimState( );

			ctx.m_cFakeData.m_flSpawnTime = ctx.m_pLocal->m_flSpawnTime( );
		}

		if ( !Interfaces::ClientState->nChokedCommands
			&& Interfaces::Globals->flCurTime != ctx.m_cFakeData.m_AnimState.flLastUpdateTime ) {
			*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cFakeData.m_AnimState;

			state->flAbsYaw = localData.m_angViewAngles.y;

			state->Update( localData.m_angViewAngles );

			ctx.m_cFakeData.m_flAbsYaw = state->flAbsYaw;
			ctx.m_cFakeData.m_AnimState = *ctx.m_pLocal->m_pAnimState( );
			ctx.m_cFakeData.m_flPoseParameter = ctx.m_pLocal->m_flPoseParameter( );

			memcpy( ctx.m_cFakeData.m_pAnimLayers, ctx.m_pLocal->m_AnimationLayers( ), 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
		}

		RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );

		ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_cFakeData.m_flPoseParameter;
		memcpy( ctx.m_pLocal->m_AnimationLayers( ), ctx.m_cFakeData.m_pAnimLayers, 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );

		ctx.m_pLocal->SetAbsAngles( { 0.f, ctx.m_cFakeData.m_flAbsYaw, 0.f } );

		SetupBonesFixed( ctx.m_pLocal, ctx.m_cFakeData.m_matMatrix, 0xFFF00,
			Interfaces::Globals->flCurTime, ( INVALIDATEBONECACHE | SETUPBONESFRAME /* | NULLIK*/ | OCCLUSIONINTERP ) );
	}

	RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );

	if ( !ctx.m_bFilledAnims )
		return;

	memcpy( ctx.m_pLocal->m_AnimationLayers( ), ctx.m_pAnimationLayers, 0x38 * ctx.m_pLocal->m_iAnimationLayersCount( ) );
	ctx.m_pLocal->m_flPoseParameter( ) = ctx.m_flPoseParameter;
	ctx.m_pLocal->SetAbsAngles( { 0, ctx.m_flAbsYaw, 0 } );
	*ctx.m_pLocal->m_pAnimState( ) = ctx.m_cAnimstate;


	if ( ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND /* && a2 < 0.1f*/ ) {
		if ( ctx.m_pLocal->GetSequenceActivity( ctx.m_pLocal->m_AnimationLayers( )[ 3 ].nSequence ) == 979 ) {
			ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flCycle = 0.f;
			ctx.m_pLocal->m_AnimationLayers( )[ 3 ].flWeight = 0.f;
		}
	}

	SetupBonesFixed( ctx.m_pLocal, ctx.m_matRealLocalBones, BONE_USED_BY_ANYTHING | BONE_ALWAYS_SETUP,
		Interfaces::Globals->flCurTime, ( INVALIDATEBONECACHE | SETUPBONESFRAME  | /*NULLIK |*/ OCCLUSIONINTERP ) );

	RestoreAnims( backupLayers, backupPose, backupAbsAngle, backupAnimstate );
}