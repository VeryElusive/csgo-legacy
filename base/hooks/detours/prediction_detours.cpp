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

	if ( cctx->cmd.iTickCount >= INT_MAX ) {
		player->m_nSimulationTick( ) = Interfaces::Globals->iTickCount;

		return Features::EnginePrediction.StoreNetvars( cctx->cmd.iCommandNumber );
	}

	Features::EnginePrediction.RestoreNetvars( cctx->cmd.iCommandNumber - 1 );

	const auto BackupTickbase = player->m_nTickBase( );

	const auto& LocalData = ctx.m_cLocalData.at( cctx->cmd.iCommandNumber % 150 );
	if ( LocalData.m_flSpawnTime == player->m_flSpawnTime( )
		&& LocalData.m_bOverrideTickbase
		&& LocalData.m_iCommandNumber == cctx->cmd.iCommandNumber )
		player->m_nTickBase( ) = LocalData.m_iAdjustedTickbase;

	/*const auto weapon = player->get_weapon( );
	if ( weapon
		&& weapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER
		&& weapon->m_flPostponeFireReadyTime( ) == INT_MAX )
		weapon->m_flPostponeFireReadyTime( ) = hacks::g_eng_pred->postpone_fire_ready_time( );*/

	oPhysicsSimulate( player, time );

	if ( LocalData.m_flSpawnTime == player->m_flSpawnTime( )
		&& LocalData.m_bOverrideTickbase && LocalData.m_bRestoreTickbase
		&& LocalData.m_iCommandNumber == cctx->cmd.iCommandNumber )
		player->m_nTickBase( ) = BackupTickbase + player->m_nTickBase( ) - LocalData.m_iAdjustedTickbase;

	Features::EnginePrediction.StoreNetvars( cctx->cmd.iCommandNumber );
}