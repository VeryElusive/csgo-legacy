#include "../core/hooks.h"
#include "../context.h"
#include "../features/misc/logger.h"
#include "../features/rage/exploits.h"
#include <intrin.h>

void FASTCALL Hooks::hkProcessMovement( void* ecx, DWORD edx, CBasePlayer* basePlayer, CMoveData* moveData ) {
	static auto oProcessMovement{ DTR::ProcessMovement.GetOriginal<decltype( &hkProcessMovement )>( ) };

	// fix prediction errors when jumping
	moveData->bGameCodeMovedPlayer = false;

	oProcessMovement( ecx, edx, basePlayer, moveData );
}

void** STDCALL Hooks::hkFinishTrackPredictionErrors( CBasePlayer* pPlayer ) {
	static auto oFinishTrackPredictionErrors{ DTR::FinishTrackPredictionErrors.GetOriginal<decltype( &hkFinishTrackPredictionErrors )>( ) };

	/*if ( ctx.m_iFixedTickBase ) {
		ctx.m_pLocal->m_nTickBase( ) = ctx.m_iFixedTickBase;
		Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_iFixedTickBase );
	}*/

	return oFinishTrackPredictionErrors( pPlayer );
}

CUserCmd* FASTCALL Hooks::hkGetUserCmd( uint8_t* ecx, uint8_t* edx, int slot, int seqnr ) {
	static auto oGetUserCmd{ DTR::GetUserCmd.GetOriginal<decltype( &hkGetUserCmd )>( ) };

	if ( !ctx.m_pLocal )
		return oGetUserCmd( ecx, edx, slot, seqnr );

	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.ReturnToPerformPrediction
		&& ctx.m_cLocalData.at( seqnr % 150 ).m_bOverrideTickbase )
		ctx.m_pLocal->m_nTickBase( ) = ctx.m_cLocalData.at( seqnr % 150 ).m_iAdjustedTickbase;

	return oGetUserCmd( ecx, edx, slot, seqnr );
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