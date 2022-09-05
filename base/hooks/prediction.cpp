#include "../core/hooks.h"
#include "../context.h"
#include "../features/misc/engine_prediction.h"
#include <intrin.h>

bool FASTCALL Hooks::hkInPrediction( void* ecx, void* edx ) {
	static auto oInPrediction{ DTR::InPrediction.GetOriginal<decltype( &hkInPrediction )>( ) };

	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.SetupBonesTiming )
		return false;

	return oInPrediction( ecx, edx );
}

void FASTCALL Hooks::hkProcessMovement( void* ecx, DWORD edx, CBasePlayer* basePlayer, CMoveData* moveData ) {
	static auto oProcessMovement{ DTR::ProcessMovement.GetOriginal<decltype( &hkProcessMovement )>( ) };

	// fix prediction errors when jumping
	moveData->bGameCodeMovedPlayer = false;

	oProcessMovement( ecx, edx, basePlayer, moveData );
}

void FASTCALL Hooks::hkRunCommand( void* ecx, void* edx, CBasePlayer* player, CUserCmd* cmd, IMoveHelper* moveHelper ) {
	static auto oRunCommand{ DTR::RunCommand.GetOriginal<decltype( &hkRunCommand )>( ) };

	if ( player != ctx.m_pLocal )
		return oRunCommand( ecx, edx, player, cmd, moveHelper );

	if ( Interfaces::Globals->iTickCount + static_cast< int >( 1 / Interfaces::Globals->flIntervalPerTick ) + 8 <= cmd->iTickCount ) {
		cmd->bHasBeenPredicted = true;

		return Features::EnginePrediction.StoreNetvars( player->m_nTickBase( ) );
	}

	const auto& LocalData{ ctx.m_cLocalData.at( cmd->iCommandNumber % 150 ) };
	const auto backupTickbase{ player->m_nTickBase( ) };
	if ( LocalData.m_flSpawnTime == player->m_flSpawnTime( )
		&& LocalData.m_bOverrideTickbase
		&& LocalData.m_iCommandNumber == cmd->iCommandNumber )
		player->m_nTickBase( ) = LocalData.m_iAdjustedTickbase;

	oRunCommand( ecx, edx, player, cmd, moveHelper );

	Features::EnginePrediction.StoreNetvars( player->m_nTickBase( ) );

	if ( LocalData.m_flSpawnTime == player->m_flSpawnTime( )
		&& LocalData.m_bOverrideTickbase && LocalData.m_bRestoreTickbase
		&& LocalData.m_iCommandNumber == cmd->iCommandNumber ) {
		player->m_nTickBase( ) = backupTickbase + player->m_nTickBase( ) - LocalData.m_iAdjustedTickbase;
		Interfaces::Globals->flCurTime = TICKS_TO_TIME( player->m_nTickBase( ) );
	}
}