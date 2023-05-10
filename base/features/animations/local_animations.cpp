#include "animation.h"

void CAnimationSys::UpdateLocal( QAngle viewAngles, const bool onlyAnimState, CUserCmd& cmd ) {
	const auto state{ ctx.m_pLocal->m_pAnimState( ) };
	if ( !state )
		return;

	const auto backupCurTime{ Interfaces::Globals->flCurTime };
	const auto backupFrameTime{ Interfaces::Globals->flFrameTime };
	const auto backupHLTV{ Interfaces::ClientState->bIsHLTV };

	Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) );
	Interfaces::Globals->flFrameTime = Interfaces::Globals->flIntervalPerTick;
	Interfaces::ClientState->bIsHLTV = true;

	Interfaces::Prediction->SetLocalViewAngles( viewAngles );

	UpdateServerLayers( cmd );

	//ctx.m_pLocal->SetAbsVelocity( ctx.m_pLocal->m_vecVelocity( ) );

	state->iLastUpdateFrame = Interfaces::Globals->iFrameCount - 1;

	ctx.m_pLocal->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = true;
	ctx.m_pLocal->UpdateClientsideAnimations( );
	ctx.m_pLocal->m_bClientSideAnimation( ) = ctx.m_bUpdatingAnimations = false;

	Interfaces::Globals->flCurTime = backupCurTime;
	Interfaces::Globals->flFrameTime = backupFrameTime;
	Interfaces::ClientState->bIsHLTV = backupHLTV;
}

constexpr float CSGO_ANIM_LOWER_REALIGN_DELAY{ 1.1f };

void CAnimationSys::UpdateServerLayers( CUserCmd& cmd ) {
	const auto state{ ctx.m_pLocal->m_pAnimState( ) };

	const auto onGround{ ctx.m_pLocal->m_fFlags( ) & FL_ONGROUND };

	/* CCSGOPlayerAnimState::SetUpVelocity */

	// i skipped ANIMATION_LAYER_ADJUST cuz it looks ugly

	if ( onGround ) {
		if ( ctx.m_pLocal->m_vecAbsVelocity( ).Length2D( ) > 0.1f ) {
			m_flLowerBodyRealignTimer = Interfaces::Globals->flCurTime + ( CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f );
			ctx.m_pLocal->m_flLowerBodyYawTarget( ) = cmd.viewAngles.y;
		}
		else {
			if ( Interfaces::Globals->flCurTime > m_flLowerBodyRealignTimer && std::abs( Math::AngleDiff( state->flAbsYaw, cmd.viewAngles.y ) ) > 35.0f ) {
				m_flLowerBodyRealignTimer = Interfaces::Globals->flCurTime + CSGO_ANIM_LOWER_REALIGN_DELAY;
				ctx.m_pLocal->m_flLowerBodyYawTarget( ) = cmd.viewAngles.y;
			}
		}
	}

	/* do weapon */

	/* CCSGOPlayerAnimState::SetUpMovement */
	auto& MOVEMENT_LAND_OR_CLIMB{ ctx.m_pLocal->m_AnimationLayers( )[ 5 ] };
	auto& MOVEMENT_JUMP_OR_FALL{ ctx.m_pLocal->m_AnimationLayers( )[ 4 ] };


	const auto onLadder{ !onGround && ctx.m_pLocal->m_MoveType( ) == MOVETYPE_LADDER };

	const auto startedLadderingThisFrame = !state->bOnLadder && onLadder;
	const auto stoppedLadderingThisFrame = state->bOnLadder && !onLadder;
	const auto landedOnGroundThisFrame{ state->bOnGround != onGround && onGround };
	const auto leftGroundThisFrame{ state->bOnGround != onGround && !onGround };

	// this will be off by 1 tick (it doesnt really make much of a difference)
	if ( state->m_flLadderWeight > 0 || onLadder ) {
		if ( startedLadderingThisFrame )
			state->SetLayerSequence( &MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_CLIMB_LADDER );
	}

	static bool landing{ };

	if ( onGround ) {
		if ( !landing && ( landedOnGroundThisFrame || stoppedLadderingThisFrame ) ) {
			state->SetLayerSequence( &MOVEMENT_LAND_OR_CLIMB, state->flDurationInAir > 1.f ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT );
			landing = true;
		}

		if ( landing && ctx.m_pLocal->GetSequenceActivity( MOVEMENT_LAND_OR_CLIMB.nSequence ) != ACT_CSGO_CLIMB_LADDER ) {
			m_bJumping = false;

			if ( MOVEMENT_JUMP_OR_FALL.flCycle + Interfaces::Globals->flFrameTime * MOVEMENT_JUMP_OR_FALL.flPlaybackRate >= 1.f )
				landing = false;
		}

		// this uses new m_bLanding
		// ladder weight not set leading to 1 tick difference
		if ( !landing && !m_bJumping && state->m_flLadderWeight <= 0 )
			MOVEMENT_LAND_OR_CLIMB.flWeight = 0;
	}
	else if ( !onLadder ) {
		landing = false;

		if ( leftGroundThisFrame || stoppedLadderingThisFrame ) {
			if ( !m_bJumping )
				state->SetLayerSequence( &MOVEMENT_JUMP_OR_FALL, ACT_CSGO_FALL );
		}
	}

	/* do SetUpFlinch/SetUpWeaponAction */

	/* SetUpLean */
	//ctx.m_pLocal->m_AnimationLayers( )[ 12 ].nSequence = 7;

	if ( cmd.iButtons & IN_JUMP && ( onGround || onLadder ) ) {
		state->SetLayerSequence( &MOVEMENT_JUMP_OR_FALL, ACT_CSGO_JUMP );
		m_bJumping = true;
	}
}