#include "../../core/hooks.h"
#include "../../context.h"
#include "../../features/misc/engine_prediction.h"
#include <intrin.h>

void FASTCALL Hooks::hkPhysicsSimulate( CBasePlayer* player, int time ) {
	static auto oPhysicsSimulate = DTR::PhysicsSimulate.GetOriginal<decltype( &hkPhysicsSimulate )>( );
	if ( player != ctx.m_pLocal
		|| player->IsDead( )
		|| Interfaces::Globals->iTickCount == player->m_nSimulationTick( ) ) {
		oPhysicsSimulate( player, time );
		return;
	}

	//player->m_vphysicsCollisionState( ) = 0;

	auto cctx = &player->m_CmdContext( );

	if ( Interfaces::Globals->iTickCount + static_cast< int >( 1 / Interfaces::Globals->flIntervalPerTick ) + 8 <= cctx->cmd.iTickCount ) {
		player->m_nSimulationTick( ) = Interfaces::Globals->iTickCount;

		return Features::EnginePrediction.StoreNetvars( cctx->cmd.iCommandNumber );
	}

	Features::EnginePrediction.RestoreNetvars( cctx->cmd.iCommandNumber - 1 );


	/*const auto weapon = player->get_weapon( );
	if ( weapon
		&& weapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER
		&& weapon->m_flPostponeFireReadyTime( ) == INT_MAX )
		weapon->m_flPostponeFireReadyTime( ) = hacks::g_eng_pred->postpone_fire_ready_time( );*/

	oPhysicsSimulate( player, time );

	//if ( ctx.m_iFixedTickBase )
	//	ctx.m_iFixedTickBase = 0;

	Features::EnginePrediction.StoreNetvars( cctx->cmd.iCommandNumber );
}

bool FASTCALL Hooks::hkPreThink( void* ecx, int edx, int a2 ) {
	static auto oPreThink = DTR::PreThink.GetOriginal<decltype( &hkPreThink )>( );

	const auto backupCurtime{ Interfaces::Globals->flCurTime };

	//if ( ctx.m_iFixedTickBase )
	//	Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_iFixedTickBase );

	const auto ret{ oPreThink( ecx, edx, a2 ) };

	Interfaces::Globals->flCurTime = backupCurtime;

	return ret;
}

bool FASTCALL Hooks::hkItemPostFrame( void* ecx, int edx ) {
	static auto oItemPostFrame = DTR::ItemPostFrame.GetOriginal<decltype( &hkItemPostFrame )>( );

	const auto backupCurtime{ Interfaces::Globals->flCurTime };

	/*if ( ctx.m_iNextTickBase ) {
		auto& oldLocalData{ ctx.m_cLocalData.at( Interfaces::ClientState->iLastCommandAck % 150 ) };
		const auto delta{ ctx.m_pLocal->m_nTickBase( ) - ( oldLocalData.m_iTickbase + 1 ) };

		Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_iNextTickBase + delta );
	}

	const auto ret{ oItemPostFrame( ecx, edx ) };

	Interfaces::Globals->flCurTime = backupCurtime;

	return ret;*/

	return 1;
}

void FASTCALL Hooks::hkSelectItem( void* ecx, int edx, int a2 ) {
	/*static auto oSelectItem = DTR::SelectItem.GetOriginal<decltype( &hkSelectItem )>( );
	
	const auto backupCurtime{ Interfaces::Globals->flCurTime };

	if ( ctx.m_iFixedTickBase )
		Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_iFixedTickBase );

	oSelectItem( ecx, edx, a2 );

	Interfaces::Globals->flCurTime = backupCurtime; */
}